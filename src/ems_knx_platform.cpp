#include "ems_knx_platform.h"

#include <Arduino.h>
#include "emsesp.h"

#include "knx/bits.h"
#include <ETH.h>

#define KNX_NETIF ETH



EMSEsp32Platform::EMSEsp32Platform() : ArduinoPlatform()
{
}

uint32_t EMSEsp32Platform::currentIpAddress()
{
    return KNX_NETIF.localIP();
}

uint32_t EMSEsp32Platform::currentSubnetMask()
{
    return KNX_NETIF.subnetMask();
}

uint32_t EMSEsp32Platform::currentDefaultGateway()
{
    return KNX_NETIF.gatewayIP();
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
    _udp = new WiFiUDP;
    LOG_DEBUG("setup multicast addr: %s port: %d ip: %s\n", mcastaddr.toString().c_str(), port, KNX_NETIF.localIP().toString().c_str());
    KNX_DEBUG_SERIAL.printf("setup multicast addr: %s port: %d ip: %s\n", mcastaddr.toString().c_str(), port,
        KNX_NETIF.localIP().toString().c_str());
    //uint8_t result = _udp->begin(port); 
    uint8_t result = _udp->beginMulticast(IPAddress(224,0,23,12), 3671); //beginMulticast(mcastaddr, port);
    LOG_DEBUG("result %d\n", result);
    KNX_DEBUG_SERIAL.printf("result %d\n", result);
}

void EMSEsp32Platform::closeMultiCast()
{
    _udp->stop();
}

bool EMSEsp32Platform::sendBytesMultiCast(uint8_t * buffer, uint16_t len)
{
    _udp->beginMulticastPacket();
    _udp->write(buffer, len);
    _udp->endPacket();
    return true;
}

int EMSEsp32Platform::readBytesMultiCast(uint8_t * buffer, uint16_t maxLen)
{
    
    int len = _udp->parsePacket();
    if (len == 0)
        return 0;

    KNX_DEBUG_SERIAL.println("readBytesMultiCast");

    if (len > maxLen) {
        LOG_DEBUG("udp buffer to small. was %d, needed %d\n", maxLen, len);
        KNX_DEBUG_SERIAL.printf("udp buffer to small. was %d, needed %d\n", maxLen, len);
        return 0;
    }

    _udp->read(buffer, len);
    return len;
}

bool EMSEsp32Platform::sendBytesUniCast(uint32_t addr, uint16_t port, uint8_t* buffer, uint16_t len)
{
    IPAddress ucastaddr(htonl(addr));

    println("sendBytesUniCast ");
    if (_udp->beginPacket(ucastaddr, port) == 1) {
        _udp->write(buffer, len);
        if (_udp->endPacket() == 0) {
            LOG_DEBUG("sendBytesUniCast endPacket fail");
        }
    } else {
        LOG_DEBUG("sendBytesUniCast beginPacket fail");
    }
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
