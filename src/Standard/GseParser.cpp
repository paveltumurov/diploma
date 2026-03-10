#include "GseParser.h"

StandardGSEParser::StandardGSEParser() : sendSocket(INVALID_SOCKET), forwardingEnabled(false) {}

StandardGSEParser::StandardGSEParser(SOCKET sock, sockaddr_in addr): sendSocket(sock), destinationAddress(addr), forwardingEnabled(true) {}

uint16_t StandardGSEParser::get12bitLength(std::uint8_t b1, std::uint8_t b2) const{
    return ((b1 & 0x0F) << 8) | b2;
}

bool StandardGSEParser::isGSEPadding(std::uint8_t byte) const{
    return (byte >> 4) == 0;
}

uint8_t StandardGSEParser::getLabelType(uint8_t b1) const{
    return (b1 >> 4) & 0b11;
}

bool StandardGSEParser::isStartFragment(uint8_t b1) const{
    return (b1 & 0x80) != 0;
}

bool StandardGSEParser::isEndFragment(uint8_t b1) const{
    return (b1 & 0x40) != 0;
}

int32_t StandardGSEParser::getHeaderLength(uint8_t b1, uint8_t b2) const{
    static constexpr int32_t LABEL_LENGTH_6_BYTES = 6;
    static constexpr int32_t LABEL_LENGTH_3_BYTES = 3;
    static constexpr int32_t BASE_HEADER_LENGTH = 2;
    static constexpr int32_t FRAGMENT_ID_FIELD_LENGTH = 1;
    static constexpr int32_t TOTAL_LENGTH_FIELD_LENGTH = 2;
    static constexpr int32_t PROTOCOL_TYPE_FIELD_LENGTH = 2;
    static constexpr int32_t CRC_FIELD_LENGTH = 4;

    int32_t gseHeaderLength = 0;
    int32_t labelLength = 0;
    uint8_t labelType = getLabelType(b1);
    if(labelType == 0b00){
        labelLength = LABEL_LENGTH_6_BYTES;
    }
    else if( labelType == 0b01){
        labelLength = LABEL_LENGTH_3_BYTES;
    }
    bool S = isStartFragment(b1);
    bool E = isEndFragment(b1);
    if(S && E){
        gseHeaderLength = BASE_HEADER_LENGTH + PROTOCOL_TYPE_FIELD_LENGTH + labelLength; 
    }
    else if(S && !E){
        gseHeaderLength = BASE_HEADER_LENGTH + PROTOCOL_TYPE_FIELD_LENGTH + FRAGMENT_ID_FIELD_LENGTH + TOTAL_LENGTH_FIELD_LENGTH + labelLength;
    }
    else if(!S  && E){
        gseHeaderLength = BASE_HEADER_LENGTH + FRAGMENT_ID_FIELD_LENGTH + CRC_FIELD_LENGTH;
    }
    else if(!S && !E){
        gseHeaderLength = BASE_HEADER_LENGTH + FRAGMENT_ID_FIELD_LENGTH;
    }
    return gseHeaderLength;
}

StandardGSEParser::~StandardGSEParser() {
    stats.paddingRatio = 100.0 * stats.totalPaddingLength / stats.totalLength;
    stats.efficiency = 100.0 * stats.totalPayloadLength / stats.totalLength;
    stats.overheadRatio = 100.0 * stats.totalHeaderLength / stats.totalLength;
    printStatistics();

    if (sendSocket != INVALID_SOCKET) {
        closesocket(sendSocket);    // Windows
        // close(sendSocket);       // Linux
        sendSocket = INVALID_SOCKET;
    }
};

void StandardGSEParser::processBBF(const char* buffer, int32_t bbfLength){
    static constexpr int32_t BBFRAME_HEADER_LENGTH = 10;
    static constexpr int32_t BASE_HEADER_LENGTH = 2;

    stats.totalLength += bbfLength - BBFRAME_HEADER_LENGTH;
    int32_t pos = BBFRAME_HEADER_LENGTH;
    while (pos + 1 <= bbfLength) {

        std::uint8_t b1 = (std::uint8_t)buffer[pos];
        if(isGSEPadding(b1)){
            stats.totalPaddingLength += bbfLength - pos;
            break;
        }

        std::uint8_t b2 = (std::uint8_t)buffer[pos + 1];
        std::uint16_t dataLength = get12bitLength(b1, b2);

        int32_t gseHeaderLength = 0;
        gseHeaderLength = getHeaderLength(b1, b2);
        stats.totalHeaderLength += gseHeaderLength;

        int32_t gsePayload = dataLength - (gseHeaderLength - BASE_HEADER_LENGTH);
        stats.totalPayloadLength += gsePayload;

        if (forwardingEnabled) {
            int32_t payloadStart = pos + gseHeaderLength;
            sendPayload(buffer + payloadStart, gsePayload);
        }

        stats.gseAmount++;
        
        pos += BASE_HEADER_LENGTH + dataLength;
    }
    std::cout<<stats.bbfNum<<std::endl;
    stats.bbfNum++;
}

void StandardGSEParser::sendPayload(const char* data, int32_t length) {
    
    int result = sendto(sendSocket, data, length, 0, (sockaddr*)&destinationAddress, sizeof(destinationAddress));
    
    if (result == SOCKET_ERROR) {
        std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
    }
}


void StandardGSEParser::printStatistics() const{
    // Заголовок
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "STATISTICS FOR Standard GSE"  << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    // Настройка форматирования
    std::cout << std::fixed << std::setprecision(4);
    
    // Ширина для выравнивания
    const int32_t labelWidth = 20;
    
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
    
    // Проверка на ошибки (если сумма не сходится)
    int32_t calculatedTotal = stats.totalPayloadLength + stats.totalHeaderLength + stats.totalPaddingLength;
    if (calculatedTotal != stats.totalLength) {
        std::cout << std::string(60, '-') << std::endl;
        std::cout << std::setw(labelWidth) << "WARNING:" << "Statistics mismatch!" << std::endl;
        std::cout << std::setw(labelWidth) << "Calculated total:" << calculatedTotal << std::endl;
        std::cout << std::setw(labelWidth) << "Difference:" << (stats.totalLength - calculatedTotal) << std::endl;
    }
    
    std::cout << std::string(60, '=') << "\n" << std::endl;
};