#include "EastarParser.h"

bool EastarGSEParser::isStartFragment(uint8_t b1){
    return (b1 & 0b00010000);
}

bool EastarGSEParser::isEndFragment(uint8_t b1){
    return (b1 & 0b00001000);
}

std::uint16_t EastarGSEParser::getLength(uint8_t b1, uint8_t b2){
    return ((b1 & 0b111) << 8) | b2;
}

bool EastarGSEParser::isGsePadding(uint8_t b1, uint8_t b2){
    return (b1 == 0 && b2 == 0);
}

EastarGSEParser::EastarGSEParser(SOCKET sock, sockaddr_in addr) : sendSocket(sock), destinationAddress(addr), forwardingEnabled(true) {}

EastarGSEParser::EastarGSEParser() : forwardingEnabled(false) {}

void EastarGSEParser::processBBF(const char* buffer, int bbfLength){

    static constexpr int32_t BBF_HEADER_LENGTH = 10;
    static constexpr int32_t BASE_GSE_HEADER_LENGTH = 2;
    static constexpr int32_t SVLAN_TYPE_FIELD_LENGTH = 4;
    static constexpr int32_t ALIGNMENT_PADDING = 1; // Паддинг для выравнивания до четности

    if(stats.bbfNum == 0){
        startTime = std::chrono::steady_clock::now();
    }
    stats.totalLength += bbfLength - BBF_HEADER_LENGTH;

    int32_t pos = BBF_HEADER_LENGTH;
    int32_t packetsInBBF = 0;
    while(pos + 1 <= bbfLength){
        int32_t gseHeaderLength = 0;
        std::uint8_t b2 = (std::uint8_t)buffer[pos];
        std::uint8_t b1 = (std::uint8_t)buffer[pos + 1];// байты поменяны местами, потому что так сделал дефолтный eastar

        if(isGsePadding(b1, b2)){
            stats.totalPaddingLength += bbfLength - pos;
            break;
        }

        std::uint16_t dataLength = getLength(b1,b2);
        int32_t gsePayloadLength = 0;

        if(isStartFragment(b1)){
            gseHeaderLength = BASE_GSE_HEADER_LENGTH + SVLAN_TYPE_FIELD_LENGTH;
            gsePayloadLength = dataLength - SVLAN_TYPE_FIELD_LENGTH;
        }
        else{
            gseHeaderLength = BASE_GSE_HEADER_LENGTH;
            gsePayloadLength = dataLength;
        }

        stats.totalHeaderLength += gseHeaderLength;
        stats.totalPayloadLength += gsePayloadLength;

        if(forwardingEnabled){
            int32_t payloadStart = pos + gseHeaderLength;
            sendPayload(buffer + payloadStart, gsePayloadLength);
        }

        if (dataLength % 2 == 1) {
            stats.totalPaddingLength += ALIGNMENT_PADDING;
            pos += BASE_GSE_HEADER_LENGTH + dataLength + ALIGNMENT_PADDING;
        }
        else{
            pos += BASE_GSE_HEADER_LENGTH + dataLength; 
        }

        packetsInBBF++;
        stats.gseAmount++;
        
    }
    stats.bbfNum++;
    
    endTime = std::chrono::steady_clock::now();
}

void EastarGSEParser::sendPayload(const char* data, int length){

    int result = sendto(sendSocket, data , length, 0, (sockaddr*)&destinationAddress, sizeof(destinationAddress));

    if (result == SOCKET_ERROR) {
        std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
    }

}


void EastarGSEParser::printStatistics() {
    stats.paddingRatio = 100.0 * stats.totalPaddingLength / stats.totalLength;
    stats.efficiency = 100.0 * stats.totalPayloadLength / stats.totalLength;
    stats.overheadRatio = 100.0 * stats.totalHeaderLength / stats.totalLength;

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    double goodput = 0;
    double throughput = 0;

    throughput = stats.totalLength / (duration.count() / 1000.0);
    goodput = stats.totalPayloadLength / (duration.count() / 1000.0);

    // Заголовок
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "STATISTICS FOR EastarGSE"  << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    // Настройка форматирования
    std::cout << std::fixed << std::setprecision(4);
    
    // Ширина для выравнивания
    const int labelWidth = 20;
    
    // Основные счетчики
    std::cout << std::left;
    std::cout << std::setw(labelWidth) << "BBF processed:" << stats.bbfNum << std::endl;
    std::cout << std::setw(labelWidth) << "GSE packets:" << stats.gseAmount << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    // Байты
    std::cout << std::setw(labelWidth) << "Total bytes:" << stats.totalLength << std::endl;
    std::cout << std::setw(labelWidth) << "Payload bytes:" << stats.totalPayloadLength << std::endl;
    std::cout << std::setw(labelWidth) << "Header bytes:" << stats.totalHeaderLength << std::endl;
    std::cout << std::setw(labelWidth) << "Padding bytes:" << stats.totalPaddingLength << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    // Соотношения (в процентах)
    std::cout << std::setw(labelWidth) << "Efficiency:" << stats.efficiency << "%" << std::endl;
    std::cout << std::setw(labelWidth) << "Overhead ratio:" << stats.overheadRatio << "%" << std::endl;
    std::cout << std::setw(labelWidth) << "Padding ratio:" << stats.paddingRatio << "%" << std::endl;
    std::cout << std::setw(labelWidth) << "Throughput:" << throughput << " byte/sec" << std::endl;
    std::cout << std::setw(labelWidth) << "Goodput:" << goodput << " byte/sec" << std::endl;
    std::cout << std::setw(labelWidth) << "Avg Time per BBF: " << (duration.count()) / (double)stats.bbfNum << std::endl;
    std::cout << std::setw(labelWidth) << "Avg Time per GSE: " << (duration.count()) / (double)stats.gseAmount << std::endl;

    // Проверка на ошибки (если сумма не сходится)
    int calculated_total = stats.totalPayloadLength + stats.totalHeaderLength + stats.totalPaddingLength;
    if (calculated_total != stats.totalLength) {
        std::cout << std::string(60, '-') << std::endl;
        std::cout << std::setw(labelWidth) << "WARNING:" << "Statistics mismatch!" << std::endl;
        std::cout << std::setw(labelWidth) << "Calculated total:" << calculated_total << std::endl;
        std::cout << std::setw(labelWidth) << "Difference:" << (stats.totalLength - calculated_total) << std::endl;
    }
    
    std::cout << std::string(60, '=') << "\n" << std::endl;
};


EastarGSEParser::~EastarGSEParser() {};

void EastarGSEParser::resetStatistics() {
    stats.bbfNum = 0;
    stats.gseAmount = 0;
    stats.totalLength = 0;
    stats.totalPayloadLength = 0;
    stats.totalHeaderLength = 0;
    stats.totalPaddingLength = 0;
    stats.efficiency = 0.0;
    stats.overheadRatio = 0.0;
    stats.paddingRatio = 0.0;
}