#include "ems_knx_platform.h"

#include <Arduino.h>
#include "emsesp.h"

#include "knx/bits.h"



EMSEsp32Platform::EMSEsp32Platform() : ArduinoPlatform()
{
}

EMSEsp32Platform::EMSEsp32Platform(HardwareSerial* s) : ArduinoPlatform(s)
{
}

uint32_t EMSEsp32Platform::currentIpAddress()
{
    return WiFi.localIP();
}

uint32_t EMSEsp32Platform::currentSubnetMask()
{
    return WiFi.subnetMask();
}

uint32_t EMSEsp32Platform::currentDefaultGateway()
{
    return WiFi.gatewayIP();
}

void EMSEsp32Platform::macAddress(uint8_t * addr)
{
    esp_wifi_get_mac(WIFI_IF_STA, addr);
}

uint32_t EMSEsp32Platform::uniqueSerialNumber()
{
    uint64_t chipid = ESP.getEfuseMac();
    uint32_t upperId = (chipid >> 32) & 0xFFFFFFFF;
    uint32_t lowerId = (chipid & 0xFFFFFFFF);
    return (upperId ^ lowerId);
}

void EMSEsp32Platform::restart()
{
    println("restart");
    ESP.restart();
}

void EMSEsp32Platform::setupMultiCast(uint32_t addr, uint16_t port)
{
    IPAddress mcastaddr(htonl(addr));
    
    KNX_DEBUG_SERIAL.printf("setup multicast addr: %s port: %d ip: %s\n", mcastaddr.toString().c_str(), port,
        WiFi.localIP().toString().c_str());
    uint8_t result = _udp.beginMulticast(mcastaddr, port);
    KNX_DEBUG_SERIAL.printf("result %d\n", result);
}

void EMSEsp32Platform::closeMultiCast()
{
    _udp.stop();
}

bool EMSEsp32Platform::sendBytesMultiCast(uint8_t * buffer, uint16_t len)
{
    //printHex("<- ",buffer, len);
    _udp.beginMulticastPacket();
    _udp.write(buffer, len);
    _udp.endPacket();
    return true;
}

int EMSEsp32Platform::readBytesMultiCast(uint8_t * buffer, uint16_t maxLen)
{
    int len = _udp.parsePacket();
    if (len == 0)
        return 0;
    
    if (len > maxLen)
    {
        KNX_DEBUG_SERIAL.printf("udp buffer to small. was %d, needed %d\n", maxLen, len);
        fatalError();
    }

    _udp.read(buffer, len);
    //printHex("-> ", buffer, len);
    return len;
}

bool EMSEsp32Platform::sendBytesUniCast(uint32_t addr, uint16_t port, uint8_t* buffer, uint16_t len)
{
    IPAddress ucastaddr(htonl(addr));
    println("sendBytesUniCast endPacket fail");
    if(_udp.beginPacket(ucastaddr, port) == 1) {
        _udp.write(buffer, len);
        if(_udp.endPacket() == 0) println("sendBytesUniCast endPacket fail");
    }
    else println("sendBytesUniCast beginPacket fail");
    return true;
}

uint8_t * EMSEsp32Platform::getEepromBuffer(size_t size) {
    if (eepromBuf_ != nullptr) {
        delete[] eepromBuf_;
    }
    eepromBuf_  = new uint8_t[size];
    eepromSize_ = size;
    emsesp::EMSESP::nvs_.getBytes("knx", eepromBuf_, size);
    return eepromBuf_;
}

void EMSEsp32Platform::commitToEeprom() {
    emsesp::EMSESP::nvs_.putBytes("knx", eepromBuf_, eepromSize_);
}
