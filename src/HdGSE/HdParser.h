#pragma once

#include "../../include/IFaces/IGseParser.h"
class HdGSEParser: public IParser{
  private:

    uint16_t getBzReservedLength(std::uint8_t b1, std::uint8_t b2) const;
    uint16_t get12bitLength(std::uint8_t b1, std::uint8_t b2) const;
    bool isGsePadding(std::uint8_t b1, std::uint8_t b2) const;

    bool isStartFragment(unsigned char b1) const;
    bool isEndFragment(unsigned char b1) const;

    enum class GseMode {
        Default = 0b00,
        Guarantee = 0b01,
        NCR = 0b10,
        CryptorAndGuarantee = 0b11
    };
    GseMode getGseMode(uint8_t b1) const;

    GSEStatistics stats;

    SOCKET sendSocket;
    sockaddr_in destinationAddress;

    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;
    bool forwardingEnabled;

  public:

    HdGSEParser();
    HdGSEParser(SOCKET sock, sockaddr_in addr);

    void processBBF(const char* buffer, int size);
    void sendPayload(const char* buffer, int size);

    void printStatistics();
    void resetStatistics();

    ~HdGSEParser() override;
};
