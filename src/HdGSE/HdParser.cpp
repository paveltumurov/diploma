#include "HdParser.h"

uint16_t HdGSEParser::getBzReservedLength(std::uint8_t b1, std::uint8_t b2) const{
    return ((b1 & 0b00011111) << 8) | b2;
}

uint16_t HdGSEParser::get12bitLength(std::uint8_t b1, std::uint8_t b2) const{
    return ((b1 & 0b00001111) << 8) | b2;
}

bool HdGSEParser::isGsePadding(std::uint8_t b1, std::uint8_t b2) const{
    return (b1 == 0) && (b2 == 0); 
}

bool HdGSEParser::isStartFragment(unsigned char b1) const{
    return (b1 & 0x80) != 0;
}

bool HdGSEParser::isEndFragment(unsigned char b1) const{
    return (b1 & 0x40) != 0;
}

HdGSEParser::GseMode HdGSEParser::getGseMode(uint8_t firstByte) const {
    uint8_t modeBits = (firstByte & 0b00110000) >> 4;
    return static_cast<GseMode>(modeBits);
}

HdGSEParser::HdGSEParser(SOCKET sock, sockaddr_in addr) : sendSocket(sock), destinationAddress(addr), forwardingEnabled(true) {}

HdGSEParser::HdGSEParser() : forwardingEnabled(false) {}

HdGSEParser::~HdGSEParser() {
    stats.paddingRatio = 100.0 * stats.totalPaddingLength / stats.totalLength;
    stats.efficiency = 100.0 * stats.totalPayloadLength / stats.totalLength;
    stats.overheadRatio = 100.0 * stats.totalHeaderLength / stats.totalLength;
    printStatistics();
};

void HdGSEParser::processBBF(const char* buffer, int bbfLength){

    static constexpr int32_t BBF_HEADER_LENGTH = 10;
    static constexpr int32_t BASE_GSE_HEADER_LENGTH = 2;
    static constexpr int32_t FRAG_ID_FIELD_LENGTH = 1;
    static constexpr int32_t PROTOCOLTYPE_FIELD_LENGTH = 2;
    static constexpr int32_t SEQUENCE_COUNTER_FIELD_LENGTH = 2;
    static constexpr int32_t HID_FIELD_LENGTH = 2;
    static constexpr int32_t BASE_BZ_HEADER_LENGTH = 2;

    stats.totalLength += bbfLength - BBF_HEADER_LENGTH;

    int pos = BBF_HEADER_LENGTH;

    while (pos + 1 <= bbfLength) {
        
        int gseHeaderLength = BASE_GSE_HEADER_LENGTH;

        std::uint8_t b1 = (std::uint8_t)buffer[pos];
        std::uint8_t b2 = (std::uint8_t)buffer[pos + 1];

        if(isGsePadding(b1, b2)){
            stats.totalPaddingLength += bbfLength - pos;
            break;
        }

        std::uint16_t dataLength = get12bitLength(b1, b2);

        std::cout << "dataLength: "<<dataLength ;

        bool S = isStartFragment(b1);
        bool E = isEndFragment(b1);

        if(S != E){
            gseHeaderLength += FRAG_ID_FIELD_LENGTH;
        }
        if(S == 1){
            gseHeaderLength += PROTOCOLTYPE_FIELD_LENGTH;
        }

        int32_t guaranteeHeaderLength = 0; // поля Sequence counter и HID
        int32_t bzHeaderLength = 0;
        GseMode mode = getGseMode(b1);
        if(mode == GseMode::Default){
            guaranteeHeaderLength = 0;
        }
        else if(mode == GseMode::Guarantee){
            guaranteeHeaderLength += SEQUENCE_COUNTER_FIELD_LENGTH + HID_FIELD_LENGTH;
        }
        else if(mode == GseMode::NCR){
            guaranteeHeaderLength = 0;
        }
        else if(mode == GseMode::CryptorAndGuarantee){

            guaranteeHeaderLength = SEQUENCE_COUNTER_FIELD_LENGTH + HID_FIELD_LENGTH;
            int32_t bzReservedLength = 0;
            int32_t bzPosition = pos + gseHeaderLength + guaranteeHeaderLength;
            uint8_t bzByte1 = buffer[bzPosition];
            uint8_t bzByte2 = buffer[bzPosition + 1];

            std::cout << " bzPosition "<< bzPosition;
            bzReservedLength = getBzReservedLength(bzByte1, bzByte2);
            bzHeaderLength = BASE_BZ_HEADER_LENGTH + bzReservedLength;

        }

        std::cout << " gseHeaderLength : " << gseHeaderLength << " guaranteeHeaderLength : " << guaranteeHeaderLength << " bz " << bzHeaderLength << std::endl;

        gseHeaderLength += guaranteeHeaderLength + bzHeaderLength;


        stats.totalHeaderLength += gseHeaderLength;

        int gsePayload = dataLength - (gseHeaderLength - BASE_GSE_HEADER_LENGTH);
        stats.totalPayloadLength += gsePayload;

        if(forwardingEnabled){
            int payloadStart = pos + gseHeaderLength;
            sendPayload(buffer + payloadStart, gsePayload);
        }

        stats.gseAmount++;
        
        pos += BASE_GSE_HEADER_LENGTH + dataLength;
    }
    
    std::cout<<stats.bbfNum<<std::endl;
    stats.bbfNum++;
}

void HdGSEParser::sendPayload(const char* data, int length){

    int result = sendto(sendSocket, data , length, 0, (sockaddr*)&destinationAddress, sizeof(destinationAddress));

    if (result == SOCKET_ERROR) {
        std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
    }

} 

void HdGSEParser::printStatistics() const{
    // Заголовок
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "STATISTICS FOR HdGSE"  << std::endl;
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