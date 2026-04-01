#pragma once

#include <string>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <winsock2.h>
#include "../../include/IFaces/IGseParser.h"

class StandardGSEParser: public IParser{
  private:
    uint16_t get12bitLength(std::uint8_t b1, std::uint8_t b2) const;
    bool isGSEPadding(std::uint8_t byte) const;
    uint8_t getLabelType(uint8_t b1) const;
    bool isStartFragment(uint8_t b1) const;
    bool isEndFragment(uint8_t b1) const;
    int getHeaderLength(uint8_t b1, uint8_t b2) const;

    SOCKET sendSocket;
    sockaddr_in destinationAddress;

    bool forwardingEnabled;

  public:
    GSEStatistics stats;

    StandardGSEParser();
    
    StandardGSEParser(SOCKET sock, sockaddr_in addr);

    void processBBF(const char* buffer, int32_t size);
    void printStatistics() ;
    void resetStatistics();
    ~StandardGSEParser() override;
    void sendPayload(const char* buffer, int32_t size);
};

