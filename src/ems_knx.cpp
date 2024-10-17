
/*
 * EMS-ESP - https://github.com/emsesp/EMS-ESP
 * Copyright 2020-2024  Paul Derbyshire
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
KNX Integration
- Remove the KNX Settings regarding Multicast IP from EMS
- Add a webpage for KNX with at least the possibility to enter prog mode and see prog mode status


*/

#include "ems_knx.h"
#include "emsesp.h"
#include <knx.h>

#include <ems_knx_platform.h>

EMSEsp32Platform knxPlatform;
Bau57B0 knxBau(knxPlatform);
KnxFacade<EMSEsp32Platform, Bau57B0> knx(knxBau);


namespace emsesp {


uuid::log::Logger Knx::logger_{"KNX", uuid::log::Facility::DAEMON};

/*
* start knx, create a multicast and unicast udp server, listen to packets
*/
bool Knx::start() {
    //_udp = new WiFiUDP;
    //if (_udp == nullptr) {
    //    LOG_ERROR("KNX failed");
    //    return false;
    //}

    knx.readMemory();   // load the stored knx configuration (from ETS) 

    GroupObject::classCallback([](GroupObject& iKo) -> void {
    //... do something with the group objects
    // iKo.asap() is the group object's number
    // iKo.value() is the group object's value
    });

    knx.start();


    xTaskCreatePinnedToCore(knx_loop_task, "knxlooptask", 4096, NULL, 2, NULL, portNUM_PROCESSORS - 1);
    LOG_INFO("KNX started");
    return true;
}

void Knx::knx_loop_task(void * pvParameters) {
    while (1) {
        static uint32_t last_loop = 0;
        if (millis() - last_loop >= LOOP_TIME) {
            last_loop = millis();
            loop();
        }
        delay(1);
    }
    vTaskDelete(NULL);
}

/*
* knx loop do something every 5 ms async.
*/
void Knx::loop() {
    // do loop
    knx.loop();

    if (!knx.configured())
        return;
    knx.getGroupObject(1).value(1, Dpt(1,0)); // just send a 1 for debug purpose to group object 1
}

/*
* called every time a value on the ems-bus or emsesp changes 
*/
bool Knx::onChange(const char * device, const char * tag, const char * name, const char * value) {
    if (tag[0] != '\0') {
        LOG_DEBUG("KNX publish: %s/%s/%s = %s", device, tag, name, value);
    } else {
        LOG_DEBUG("KNX publish: %s/%s = %s", device, name, value);
    }
    return true;
}

/*
* get a emsesp value
*/
bool Knx::getValue(const char * device, const char * tag, const char * name, char * value, size_t len) {
    char cmd[COMMAND_MAX_LENGTH];
    if (strlen(tag)) {
        snprintf(cmd, sizeof(cmd), "%s/%s/%s/value", device, tag, name);
    } else {
        snprintf(cmd, sizeof(cmd), "%s/%s/value", device, name);
    }
    JsonDocument doc_in, doc_out;
    JsonObject   in  = doc_in.to<JsonObject>();
    JsonObject   out = doc_out.to<JsonObject>();

    Command::process(cmd, true, in, out);
    if (out["api_data"] .is<std::string>()) {
        std::string data = out["api_data"];
        strlcpy(value, data.c_str(), len);
        return true;
    }
    LOG_ERROR("KNX get value failed for %s/%s/%s", device, tag, name);
    return false;
}

/*
* set an emsesp value 
*/

bool Knx::setValue(const char * device, const char * tag, const char * name, const char * value) {
    char cmd[COMMAND_MAX_LENGTH];
    if (strlen(tag)) {
        snprintf(cmd, sizeof(cmd), "%s/%s", tag, name);
    } else {
        snprintf(cmd, sizeof(cmd), "%s", name);
    }
    uint8_t devicetype = EMSdevice::device_name_2_device_type(device);
    if (Command::call(devicetype, cmd, value) != CommandRet::OK) {
        LOG_ERROR("KNX command failed for %s/%s/%s", device, tag, name);
        return false;
    }
    return true;
}

} // namespace emsesp