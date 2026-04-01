#include <string>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <winsock2.h>
#include "../../include/IFaces/IGseParser.h"
#pragma once
class EastarGSEParser : public IParser{
  private:

    SOCKET sendSocket;
    sockaddr_in destinationAddress;

    bool forwardingEnabled;

    bool isStartFragment(uint8_t b1);
    bool isEndFragment(uint8_t b1);
    bool isGsePadding(uint8_t b1, uint8_t b2);

    std::uint16_t getLength(uint8_t b1, uint8_t b2);

    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;

  public:
    EastarGSEParser(SOCKET sock, sockaddr_in addr);
    EastarGSEParser();

    GSEStatistics stats;

    void processBBF(const char* buffer, int size);
    void sendPayload(const char* buffer, int size);

    void printStatistics();
    void resetStatistics();

    ~EastarGSEParser() override;
};
