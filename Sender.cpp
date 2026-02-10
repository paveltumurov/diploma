#include <iostream>
#include <vector>
#include <winsock2.h>
#include <chrono>
#include <string>
#include <sstream>
#include "PacketGenerator.h"
#include <algorithm>
#pragma comment(lib, "ws2_32.lib")

std::unique_ptr<ILengthGenerator> chooseLengthGenerator() {
    int choice;
    std::cout << "\n=== Choose Packet Length Generator ===\n";
    std::cout << "1. Fixed length\n";
    std::cout << "2. Uniform distribution\n";
    std::cout << "3. Sequential pattern\n";
    std::cout << "4. MTU boundary testing\n";
    std::cout << "5. VoIP-like traffic\n";
    std::cout << "6. Video-like traffic\n";
    std::cout << "Choose generator type (1-6): ";
    std::cin >> choice;

    switch (choice) {
        case 1: {
            uint32_t length;
            std::cout << "Enter fixed packet length: ";
            std::cin >> length;
            return GeneratorFactory::createFixedLength(length);
        }
        case 2: {
            uint32_t minLen, maxLen;
            std::cout << "Enter minimum length: ";
            std::cin >> minLen;
            std::cout << "Enter maximum length: ";
            std::cin >> maxLen;
            return GeneratorFactory::createUniformLength(minLen, maxLen);
        }
        case 3: {
            std::vector<uint32_t> sequence;
            int count;
            std::cout << "How many lengths in sequence? ";
            std::cin >> count;
            std::cout << "Enter " << count << " lengths:\n";
            for (int i = 0; i < count; ++i) {
                uint32_t len;
                std::cin >> len;
                sequence.push_back(len);
            }
            return GeneratorFactory::createSequentialLength(sequence);
        }
        case 4: {
            std::vector<uint32_t> mtuSequence = {1490, 1495, 1499, 1500, 1501, 1505, 1510};
            std::cout << "Using MTU boundary sequence: ";
            for (auto len : mtuSequence) std::cout << len << " ";
            std::cout << "\n";
            return GeneratorFactory::createSequentialLength(mtuSequence);
        }
        case 5: {
            std::cout << "Using VoIP-like traffic (64-128 bytes)\n";
            return GeneratorFactory::createUniformLength(64, 128);
        }
        case 6: {
            std::cout << "Using Video-like traffic (500-1500 bytes)\n";
            return GeneratorFactory::createUniformLength(500, 1500);
        }
        default: {
            std::cout << "Invalid choice, using default (100-500 bytes)\n";
            return GeneratorFactory::createUniformLength(100, 500);
        }
    }
}

std::unique_ptr<IContentGenerator> chooseContentGenerator() {
    int choice;
    std::cout << "\n=== Choose Content Generator ===\n";
    std::cout << "1. Random data\n";
    std::cout << "2. Incremental data\n";
    std::cout << "3. Pattern data\n";
    std::cout << "4. All zeros\n";
    std::cout << "Choose content type (1-4): ";
    std::cin >> choice;

    switch (choice) {
        case 1:
            std::cout << "Using random content\n";
            return GeneratorFactory::createRandomContent();
        case 2: {
            uint8_t startValue;
            std::cout << "Enter start value (0-255): ";
            int temp;
            std::cin >> temp;
            startValue = static_cast<uint8_t>(temp);
            return GeneratorFactory::createIncrementalContent(startValue);
        }
        case 3: {
            std::vector<uint8_t> pattern;
            std::string patternStr;
            std::cout << "Enter pattern as hex bytes (e.g., 'AABBCC'): ";
            std::cin >> patternStr;
            
            for (size_t i = 0; i < patternStr.length(); i += 2) {
                std::string byteString = patternStr.substr(i, 2);
                uint8_t byte = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
                pattern.push_back(byte);
            }
            std::cout << "Using pattern: ";
            for (auto b : pattern) {
                std::cout << std::hex << static_cast<int>(b) << " ";
            }
            std::cout << std::dec << "\n";
            return GeneratorFactory::createPatternContent(pattern);
        }
        case 4:
            std::cout << "Using zeros content\n";
            return GeneratorFactory::createZerosContent();
        default:
            std::cout << "Invalid choice, using random content\n";
            return GeneratorFactory::createRandomContent();
    }
}

std::vector<int> chooseDelayPattern(int packetCount) {
    int choice;
    std::cout << "\n=== Choose Delay Pattern ===\n";
    std::cout << "1. Constant delay\n";
    std::cout << "2. Burst traffic \n";
    std::cout << "3. Increasing delays\n";
    std::cout << "4. Random delays\n";
    std::cout << "5. Custom pattern \n";
    std::cout << "Choose delay pattern (1-5): ";
    std::cin >> choice;

    std::vector<int> delays(packetCount, 0);

    switch (choice) {
        case 1: {
            // Постоянная задержка
            int constantDelay;
            std::cout << "Enter constant delay (ms): ";
            std::cin >> constantDelay;
            std::fill(delays.begin(), delays.end(), constantDelay);
            std::cout << "Using constant delay: " << constantDelay << "ms\n";
            break;
        }
        case 2: {
            // Burst traffic - пачки пакетов
            int burstSize, burstDelay, betweenBurstDelay;
            std::cout << "Enter packets per burst: ";
            std::cin >> burstSize;
            std::cout << "Enter delay within burst (ms): ";
            std::cin >> burstDelay;
            std::cout << "Enter delay between bursts (ms): ";
            std::cin >> betweenBurstDelay;
            
            for (int i = 0; i < packetCount; ++i) {
                if (i % burstSize == 0 && i != 0) {
                    delays[i] = betweenBurstDelay;  // Задержка между пачками
                } else {
                    delays[i] = burstDelay;  // Задержка внутри пачки
                }
            }
            std::cout << "Burst pattern: " << burstSize << " packets with " 
                      << burstDelay << "ms delay, then " << betweenBurstDelay << "ms\n";
            break;
        }
        case 3: {
            // Нарастающие задержки
            int startDelay, increment;
            std::cout << "Enter starting delay (ms): ";
            std::cin >> startDelay;
            std::cout << "Enter delay increment (ms): ";
            std::cin >> increment;
            
            for (int i = 0; i < packetCount; ++i) {
                delays[i] = startDelay + (i * increment);
            }
            std::cout << "Increasing delays: " << startDelay << "ms + " 
                      << increment << "ms per packet\n";
            break;
        }
        case 4: {
            // Случайные задержки
            int minDelay, maxDelay;
            std::cout << "Enter minimum random delay (ms): ";
            std::cin >> minDelay;
            std::cout << "Enter maximum random delay (ms): ";
            std::cin >> maxDelay;
            
            srand(static_cast<unsigned int>(time(nullptr)));
            for (int i = 0; i < packetCount; ++i) {
                delays[i] = minDelay + (rand() % (maxDelay - minDelay + 1));
            }
            std::cout << "Random delays between " << minDelay << "ms and " << maxDelay << "ms\n";
            break;
        }
        case 5: {
            // Ручной ввод
            std::cout << "Enter " << packetCount << " delay values in milliseconds:\n";
            std::cout << "You can enter: '100' or '50 50 200 200' or '10*5 100*2' for patterns\n";
            std::cout << "Delays: ";
            
            std::cin.ignore();
            std::string input;
            std::getline(std::cin, input);
            
            std::vector<int> customDelays;
            std::stringstream ss(input);
            std::string token;
            
            while (ss >> token) {
                // Проверяем формат "value*count"
                size_t starPos = token.find('*');
                if (starPos != std::string::npos) {
                    int value = std::stoi(token.substr(0, starPos));
                    int count = std::stoi(token.substr(starPos + 1));
                    for (int j = 0; j < count; ++j) {
                        customDelays.push_back(value);
                    }
                } else {
                    customDelays.push_back(std::stoi(token));
                }
            }
            
            // Если введено меньше значений - повторяем последний
            if (customDelays.size() < packetCount) {
                int lastDelay = customDelays.empty() ? 0 : customDelays.back();
                while (customDelays.size() < packetCount) {
                    customDelays.push_back(lastDelay);
                }
            }
            
            // Копируем только нужное количество
            delays.assign(customDelays.begin(), customDelays.begin() + packetCount);
            
            std::cout << "Custom delay pattern: ";
            for (int i = 0; i < std::min(10, packetCount); ++i) {
                std::cout << delays[i] << " ";
            }
            if (packetCount > 10) std::cout << "...";
            std::cout << "\n";
            break;
        }
        default: {
            std::cout << "Invalid choice, using constant 100ms delay\n";
            std::fill(delays.begin(), delays.end(), 100);
            break;
        }
    }
    
    return delays;
}

bool setupUDPSocket(SOCKET& clientSocket, sockaddr_in& serverAddr, const std::string& serverIP, int port) {
    clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "UDP socket creation failed: " << WSAGetLastError() << "\n";
        return false;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());
    
    if (serverAddr.sin_addr.s_addr == INADDR_NONE) {
        std::cerr << "Invalid IP address: " << serverIP << "\n";
        closesocket(clientSocket);
        return false;
    }
    
    std::cout << "UDP socket created for " << serverIP << ":" << port << "\n";
    return true;
}

void sendPackets(SOCKET clientSocket, sockaddr_in serverAddr, std::unique_ptr<PacketGenerator>& packetGenerator, int packetCount, const std::vector<int>& delays) {
    std::cout << "\nStarting UDP packet transmission with variable delays...\n";

    int successfullySent = 0;
    int totalDelay = 0;
    
    for (int i = 0; i < packetCount; ++i) {
        auto packet = packetGenerator->generatePacket();
        
        int sent = sendto(clientSocket, 
                         (char*)packet.data.data(), 
                         packet.data.size(),
                         0,
                         (sockaddr*)&serverAddr,
                         sizeof(serverAddr));

        if (sent == SOCKET_ERROR) {
            std::cerr << "Failed to send packet #" << packet.sequenceNumber 
                      << " - Error: " << WSAGetLastError() << "\n";
        } else {
            successfullySent++;
            std::cout << "Sent packet #" << packet.sequenceNumber 
                      << " | Length: " << packet.data.size() << " bytes"
                      << " | Next delay: " << delays[i] << "ms\n";
        }
        
        if (delays[i] > 0) {
            totalDelay += delays[i];
            Sleep(delays[i]);
        }
    }
    
    std::cout << "Transmission complete!\n";
    std::cout << "Successfully sent: " << successfullySent << "/" << packetCount << " packets\n";
    std::cout << "Total transmission time: " << totalDelay << "ms\n";
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    std::string serverIP;
    int port;

    std::cout << "=== UDP Packet Sender with Variable Delays ===\n";
    std::cout << "Enter server IP address: ";
    std::cin >> serverIP;
    std::cout << "Enter server port: ";
    std::cin >> port;

    SOCKET clientSocket;
    sockaddr_in serverAddr;
    
    while (true) {
        std::cout << "\n" << std::string(50, '=') << "\n";
        std::cout << "1. Send packets\n";
        std::cout << "2. Change server (" << serverIP << ":" << port << ")\n";
        std::cout << "3. Exit\n";
        std::cout << "Choose option (1-3): ";
        
        int mainChoice;
        std::cin >> mainChoice;

        if (mainChoice == 3) {
            std::cout << "Goodbye!\n";
            break;
        }
        else if (mainChoice == 2) {
            std::cout << "Enter new server IP: ";
            std::cin >> serverIP;
            std::cout << "Enter new port: ";
            std::cin >> port;
            continue;
        }
        else if (mainChoice == 1) {
            if (!setupUDPSocket(clientSocket, serverAddr, serverIP, port)) {
                std::cout << "Socket setup failed. Please check server address.\n";
                continue;
            }
            
            std::cout << "Ready to send to " << serverIP << ":" << port << "!\n";

            auto lengthGenerator = chooseLengthGenerator();
            auto contentGenerator = chooseContentGenerator();

            auto packetGenerator = std::unique_ptr<PacketGenerator>(
                new PacketGenerator(std::move(lengthGenerator), std::move(contentGenerator))
            );

            int packetCount;
            std::cout << "\n=== Sending Settings ===\n";
            std::cout << "How many packets to send? ";
            std::cin >> packetCount;

            // Выбираем паттерн задержек
            auto delays = chooseDelayPattern(packetCount);

            sendPackets(clientSocket, serverAddr, packetGenerator, packetCount, delays);
            
            closesocket(clientSocket);
        }
        else {
            std::cout << "Invalid option!\n";
        }
    }

    WSACleanup();
    return 0;
}