#include <iostream>
#include <winsock2.h>
#include <vector>
#include <iomanip>

#pragma comment(lib, "ws2_32.lib")

uint16_t get12bitLength(std::uint8_t b1, std::uint8_t b2) {
    return ((b1 & 0x0F) << 8) | b2;
}

bool is_gse_padding(std::uint8_t byte) {
    return (byte >> 4) == 0; // Старшие 4 бита равны 0
}
// Функция для получения битов 2-3 (Label_Type_Indicator)
unsigned char getLabelType(unsigned char b1) {
    return (b1 >> 4) & 0b11; // Бит 0 - S, бит 1 - E, биты 2-3 - LT
}

// Функция для проверки S (Start) бита
bool isStart(unsigned char b1) {
    return (b1 & 0x80) != 0; // Бит 7
}

// Функция для проверки E (End) бита
bool isEnd(unsigned char b1) {
    return (b1 & 0x40) != 0; // Бит 6
}

int main() {
    int total_payload = 0;
    int gse_amount = 0;
    int bbf_num = 0;
    int total_padding = 0;
    int total_length = 0;
    int total_header = 0;
    // Инициализация Winsock
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    
    // Сокет для приема BBF пакетов
    SOCKET recv_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (recv_sock == INVALID_SOCKET) {
        std::cerr << "Failed to create receive socket: " << WSAGetLastError() << std::endl;
        return 1;
    }
    
    sockaddr_in recv_addr;
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_addr.s_addr = INADDR_ANY;
    recv_addr.sin_port = htons(12345);
    
    if (bind(recv_sock, (sockaddr*)&recv_addr, sizeof(recv_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        return 1;
    }
    
    // Сокет для отправки отдельных пакетов
    SOCKET send_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (send_sock == INVALID_SOCKET) {
        std::cerr << "Failed to create send socket: " << WSAGetLastError() << std::endl;
        return 1;
    }
    
    sockaddr_in send_addr;
    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // localhost
    send_addr.sin_port = htons(10000);
    
    std::cout << "Listening for BBF packets on port 12345..." << std::endl;
    std::cout << "Forwarding individual packets to 127.0.0.1:10000\n" << std::endl;
    
    char buffer[65507];//массив принятых байтов(не более 65508)
    sockaddr_in sender;
    int addrLen = sizeof(sender);
    
    while (bbf_num < 13019) {
        int total = recvfrom(recv_sock, buffer, sizeof(buffer), 0, (sockaddr*)&sender, &addrLen);//размер принятого пакета
        total_length += total-10;
        if (total <= 12) {
            std::cout << "Received too small packet: " << total << " bytes" << std::endl;
            continue;
        }//проверка на слишком маленький пакет

        int packets_in_bbf = 0;
        int pos = 10; // Пропускаем BBF заголовок (10 байт)
        while (pos + 1 <= total) {
            int gse_header_length = 0;
            std::uint8_t b1 = (std::uint8_t)buffer[pos];
            if(is_gse_padding(b1)){
                total_padding += total - pos;
                break;
            }//проверка на паддинг (первые 4 бита == 0)

            std::uint8_t b2 = (std::uint8_t)buffer[pos + 1];
            std::uint16_t dataLength = get12bitLength(b1, b2);//длина gse пакета

            //Нахождение длины заголовка
            int label_size = 0;
            unsigned char labeltype = getLabelType(b1);
            if(labeltype == 0b00){
                label_size = 6;
            }
            else if( labeltype == 0b01){
                label_size = 3;
            }
            bool S = isStart(b1);
            bool E = isEnd(b1);
            if(S && E){
                gse_header_length = 4 + label_size; 
            }
            else if(S && !E){
                gse_header_length = 7 + label_size;
            }
            else if(!S  && E){
                gse_header_length = 7;
            }
            else if(!S && !E){
                gse_header_length = 3;
            }
            total_header += gse_header_length;

            //Расчет payload
            int gse_payload = dataLength + 2 - gse_header_length;
            total_payload += gse_payload;

            // Проверяем, что пакет полностью помещается в BBF
            if (pos + 2 + dataLength > total) {
                std::cout << "WARNING: Packet extends beyond BBF boundary! BBF#" 
                          << bbf_num + 1 << ", pos=" << pos 
                          << ", dataLength=" << dataLength 
                          << ", total=" << total << std::endl;
                break;
            }
            
            
            // Отправляем пакет только если есть данные
            int send_result = sendto(send_sock, &buffer[pos], 2 + dataLength, 0,
                                   (sockaddr*)&send_addr, sizeof(send_addr));
            
            if (send_result == SOCKET_ERROR) {
                int error = WSAGetLastError();
                std::cerr << "Send failed: " << error 
                          << " (packet size: " << (2 + dataLength) << ")" << std::endl;
            } else {
                packets_in_bbf++;
                gse_amount++;
            }
            
            // Переходим к следующему пакету
            pos += 2 + dataLength;
        }
        
        bbf_num++;
        
        // Выводим статистику каждые N BBF
        if (bbf_num % 10 == 0) {
            std::cout << "BBF #" << bbf_num << ": " 
                      << packets_in_bbf << " packets forwarded, "
                      << "total packets: " << gse_amount;
            std::cout << std::endl;
        }
        
        // // Дополнительный дебаг вывод при подозрительных ситуациях
        // if (packets_in_bbf == 0 && pos < total) {
        //     std::cout << "DEBUG: BBF#" << bbf_num 
        //               << " has no valid packets, but " 
        //               << (total - 10) << " bytes after header" << std::endl;
            
        //     // Выводим первые несколько байт для анализа
        //     std::cout << "First 20 bytes after BBF header: ";
        //     for (int i = 10; i < std::min(30, total); i++) {
        //         std::cout << std::hex << std::setw(2) << std::setfill('0') 
        //                   << ((unsigned int)buffer[i] & 0xFF) << " ";
        //     }
        //     std::cout << std::dec << std::endl;
        // }
    }
    closesocket(send_sock);
    closesocket(recv_sock);
    WSACleanup();

    std::cout<<"BBF numbers: "<< bbf_num<<std::endl;
    std::cout<<"Total bytes: "<< total_length<<std::endl;
    std::cout <<"Padding bytes: "<< total_padding<<std::endl;
    std::cout << "Padding ratio: " << (double)total_padding/total_length*100.0<<"%"<< std::endl;
    std::cout<< "Payload bytes: " << total_payload << std::endl;
    std::cout<< "Header bytes: " << total_header << std::endl;
    std::cout<< "Overhead ratio: " << (double)total_header/total_length*100.0<<"%" << std::endl;
    std::cout << "Efficiency: "<< (double)total_payload/total_length*100.0 << "%" << std::endl;
    std::cout << "Gse_amount: "<<gse_amount<<std::endl;
    std::cout << "Difference: " << total_length-total_header-total_payload - total_padding;
}
