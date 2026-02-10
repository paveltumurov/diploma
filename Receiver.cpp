#include <iostream>
#include <vector>
#include <winsock2.h>
#include <iomanip>
#include <string>

#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    int port;
    std::cout << "=== Packet Receiver (Always Running) ===\n";
    std::cout << "Enter port to listen on: ";
    std::cin >> port;

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);
    
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed on port " << port << " - Error: " << WSAGetLastError() << "\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    
    if (listen(serverSocket, 5) == SOCKET_ERROR) {  // Увеличили очередь до 5
        std::cerr << "Listen failed: " << WSAGetLastError() << "\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    
    std::cout << "Server listening on port " << port << "...\n";
    std::cout << "Press Ctrl+C to stop the server.\n\n";

    int sessionCounter = 0;
    
    while (true) {
        std::cout << "Waiting for client connection...\n";
        
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
        
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed: " << WSAGetLastError() << "\n";
            continue;  // Продолжаем слушать дальше
        }
        
        char* clientIP = inet_ntoa(clientAddr.sin_addr);
        sessionCounter++;
        std::cout << "\n=== Session #" << sessionCounter << " ===\n";
        std::cout << "Client connected from " << clientIP << ":" << ntohs(clientAddr.sin_port) << "\n";

        int packetCount = 0;
        bool sessionActive = true;
        
        while (sessionActive) {
            size_t dataSize;
            int bytesReceived = recv(clientSocket, (char*)&dataSize, sizeof(size_t), 0);
            
            if (bytesReceived <= 0) {
                std::cout << "Client disconnected\n";
                sessionActive = false;
                break;
            }
            
            if (dataSize == 0) {
                std::cout << "End of transmission signal received\n";
                // Не разрываем соединение, ждем новые пакеты
                continue;
            }

            std::vector<uint8_t> receivedData(dataSize);
            int totalBytesReceived = 0;
            
            while (totalBytesReceived < dataSize) {
                bytesReceived = recv(clientSocket, 
                                   (char*)receivedData.data() + totalBytesReceived, 
                                   dataSize - totalBytesReceived, 0);
                if (bytesReceived <= 0) {
                    sessionActive = false;
                    break;
                }
                totalBytesReceived += bytesReceived;
            }

            if (totalBytesReceived == dataSize) {
                packetCount++;
                std::cout << "Packet #" << packetCount << " received (" 
                          << receivedData.size() << " bytes): ";
                
                for (size_t i = 0; i < std::min<size_t>(6, receivedData.size()); ++i) {
                    std::cout << std::hex << std::setw(2) << std::setfill('0') 
                              << static_cast<int>(receivedData[i]) << " ";
                }
                if (receivedData.size() > 6) {
                    std::cout << "...";
                }
                std::cout << std::dec << "\n";
            } else if (sessionActive) {
                std::cout << "Error: incomplete packet data received\n";
            }
        }

        std::cout << "Session #" << sessionCounter << " ended. Total packets: " << packetCount << "\n\n";
        closesocket(clientSocket);
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}