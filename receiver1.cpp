#include <iostream>
#include <vector>
#include <winsock2.h>
#include <iomanip>
#include <string>
#include <algorithm>
#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    int port;
    std::cout << "=== UDP Packet Receiver (Always Running) ===\n";
    std::cout << "Enter port to listen on: ";
    std::cin >> port;

    // Создаем UDP сокет
    SOCKET serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "UDP socket creation failed: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    // Настраиваем адрес для приема
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);
    
    // Привязываем сокет к адресу
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed on port " << port << " - Error: " << WSAGetLastError() << "\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    
    std::cout << "UDP server listening on port " << port << "...\n";
    std::cout << "Press Ctrl+C to stop the server.\n\n";

    const int MAX_PACKET_SIZE = 65507;  // Максимальный размер UDP датаграммы
    std::vector<uint8_t> buffer(MAX_PACKET_SIZE);
    
    int packetCount = 0;
    
    while (true) {
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        
        // Принимаем датаграмму
        int bytesReceived = recvfrom(serverSocket, 
                                   (char*)buffer.data(), 
                                   MAX_PACKET_SIZE,
                                   0,
                                   (sockaddr*)&clientAddr, 
                                   &clientAddrSize);

        if (bytesReceived == SOCKET_ERROR) {
            std::cerr << "Receive failed: " << WSAGetLastError() << "\n";
            continue;
        }

        if (bytesReceived > 0) {
            packetCount++;
            
            char* clientIP = inet_ntoa(clientAddr.sin_addr);
            
            std::cout << "Packet #" << packetCount 
                      << " from " << clientIP << ":" << ntohs(clientAddr.sin_port)
                      << " | Size: " << bytesReceived << " bytes | ";
            
            // Выводим первые байты
            for (int i = 0; i < std::min(6, bytesReceived); ++i) {
                std::cout << std::hex << std::setw(2) << std::setfill('0') 
                          << static_cast<int>(buffer[i]) << " ";
            }
            if (bytesReceived > 6) {
                std::cout << "...";
            }
            std::cout << std::dec << "\n";
        }
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}