#include "PacketGenerator.h"
#include <iostream>
#include <iomanip>
#include <fstream>

void printPacketInfo(const PacketGenerator::GeneratedPacket& packet) {
    std::cout << "Packet #" << packet.sequenceNumber 
              << " | Length: " << packet.length << " bytes\n";
    
    // Выводим первые 16 байт содержимого
    std::cout << "First 16 bytes: ";
    for (size_t i = 0; i < std::min<size_t>(16, packet.data.size()); ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') 
                  << static_cast<int>(packet.data[i]) << " ";
    }
    std::cout << std::dec << "\n";
    
    // Выводим метаданные (исправлено для C++11)
    std::cout << "Metadata:\n";
    for (const auto& entry : packet.metadata) {
        std::cout << "  " << entry.first << ": " << entry.second << "\n";
    }
    std::cout << "---\n";
}

void testBasicGenerators() {
    std::cout << "=== TEST 1: Basic Generators ===\n";
    
    // Тест фиксированной длины + случайное содержимое
    auto fixedRandomGen = std::unique_ptr<PacketGenerator>(
        new PacketGenerator(
            GeneratorFactory::createFixedLength(100),
            GeneratorFactory::createRandomContent()
        )
    );
    
    std::cout << "Fixed length (100) + Random content:\n";
    auto packet1 = fixedRandomGen->generatePacket();
    printPacketInfo(packet1);
    
    // Тест равномерного распределения + инкрементальное содержимое
    auto uniformIncrGen = std::unique_ptr<PacketGenerator>(
        new PacketGenerator(
            GeneratorFactory::createUniformLength(50, 200),
            GeneratorFactory::createIncrementalContent(10)
        )
    );
    
    std::cout << "\nUniform length (50-200) + Incremental content:\n";
    for (int i = 0; i < 3; ++i) {
        auto packet = uniformIncrGen->generatePacket();
        printPacketInfo(packet);
    }
}

void testPatternGenerators() {
    std::cout << "\n=== TEST 2: Pattern Generators ===\n";
    
    // Тест последовательной длины + паттерн
    std::vector<uint32_t> lengthSequence;
    lengthSequence.push_back(32);
    lengthSequence.push_back(64);
    lengthSequence.push_back(128);
    lengthSequence.push_back(256);
    
    std::vector<uint8_t> contentPattern;
    contentPattern.push_back(0xAA);
    contentPattern.push_back(0xBB);
    contentPattern.push_back(0xCC);
    contentPattern.push_back(0xDD);
    
    auto sequentialPatternGen = std::unique_ptr<PacketGenerator>(
        new PacketGenerator(
            GeneratorFactory::createSequentialLength(lengthSequence),
            GeneratorFactory::createPatternContent(contentPattern)
        )
    );
    
    std::cout << "Sequential length + Pattern content:\n";
    for (int i = 0; i < 6; ++i) { // Больше чем размер последовательности
        auto packet = sequentialPatternGen->generatePacket();
        printPacketInfo(packet);
    }
}

void testZerosGenerator() {
    std::cout << "\n=== TEST 3: Zeros Generator ===\n";
    
    auto zerosGen = std::unique_ptr<PacketGenerator>(
        new PacketGenerator(
            GeneratorFactory::createFixedLength(20),
            GeneratorFactory::createZerosContent()
        )
    );
    
    std::cout << "Fixed length + Zeros content:\n";
    auto packet = zerosGen->generatePacket();
    printPacketInfo(packet);
}

void testPresetGenerators() {
    std::cout << "\n=== TEST 4: Preset Generators ===\n";
    
    // VoIP генератор
    std::cout << "VoIP Generator:\n";
    auto voipGen = GeneratorFactory::createVoIPGenerator();
    for (int i = 0; i < 2; ++i) {
        auto packet = voipGen->generatePacket();
        printPacketInfo(packet);
    }
    
    // Video Stream генератор
    std::cout << "\nVideo Stream Generator:\n";
    auto videoGen = GeneratorFactory::createVideoStreamGenerator();
    for (int i = 0; i < 2; ++i) {
        auto packet = videoGen->generatePacket();
        printPacketInfo(packet);
    }
    
    // Bursty Traffic генератор
    std::cout << "\nBursty Traffic Generator:\n";
    auto burstyGen = GeneratorFactory::createBurstyTrafficGenerator();
    for (int i = 0; i < 5; ++i) {
        auto packet = burstyGen->generatePacket();
        printPacketInfo(packet);
    }
    
    // MTU Boundary генератор
    std::cout << "\nMTU Boundary Generator:\n";
    auto mtuGen = GeneratorFactory::createMTUBoundaryGenerator();
    for (int i = 0; i < 3; ++i) {
        auto packet = mtuGen->generatePacket();
        printPacketInfo(packet);
    }
}

void testBatchGeneration() {
    std::cout << "\n=== TEST 5: Batch Generation ===\n";
    
    auto batchGen = std::unique_ptr<PacketGenerator>(
        new PacketGenerator(
            GeneratorFactory::createUniformLength(10, 30),
            GeneratorFactory::createRandomContent()
        )
    );
    
    std::cout << "Generating 5 packets in batch:\n";
    auto packets = batchGen->generatePackets(5);
    
    for (size_t i = 0; i < packets.size(); ++i) {
        printPacketInfo(packets[i]);
    }
    
    std::cout << "Total packets generated: " << batchGen->getPacketsGenerated() << "\n";
}

void testGeneratorParameters() {
    std::cout << "\n=== TEST 6: Generator Parameters ===\n";
    
    // Тест изменения параметров генератора
    auto uniformGen = GeneratorFactory::createUniformLength(10, 20);
    auto contentGen = GeneratorFactory::createIncrementalContent();
    
    auto dynamicGen = std::unique_ptr<PacketGenerator>(
        new PacketGenerator(
            std::move(uniformGen),
            std::move(contentGen)
        )
    );
    
    std::cout << "Initial range (10-20):\n";
    auto packet1 = dynamicGen->generatePacket();
    printPacketInfo(packet1);
    
    // Изменяем параметры через интерфейс
    IRandomLengthGenerator* randomLengthGen = dynamic_cast<IRandomLengthGenerator*>(
        dynamicGen->getLengthGenerator());
    if (randomLengthGen) {
        randomLengthGen->setRange(50, 100);
        std::cout << "Changed range to (50-100):\n";
    }
    
    auto packet2 = dynamicGen->generatePacket();
    printPacketInfo(packet2);
}

void savePacketsToFile(const std::vector<PacketGenerator::GeneratedPacket>& packets, 
                      const std::string& filename) {
    std::ofstream file(filename.c_str(), std::ios::binary);
    if (!file) {
        std::cerr << "Cannot open file: " << filename << "\n";
        return;
    }
    
    for (size_t i = 0; i < packets.size(); ++i) {
        const auto& packet = packets[i];
        file.write(reinterpret_cast<const char*>(packet.data.data()), packet.data.size());
    }
    
    std::cout << "Saved " << packets.size() << " packets to " << filename << "\n";
}

int main() {
    std::cout << "Packet Generator Test Program\n";
    std::cout << "=============================\n\n";
    
    try {
        testBasicGenerators();
        testPatternGenerators();
        testZerosGenerator();
        testPresetGenerators();
        testBatchGeneration();
        testGeneratorParameters();
        
        // Дополнительный тест: сохранение в файл
        std::cout << "\n=== Saving packets to file ===\n";
        auto fileGen = std::unique_ptr<PacketGenerator>(
            new PacketGenerator(
                GeneratorFactory::createUniformLength(100, 200),
                GeneratorFactory::createRandomContent()
            )
        );
        auto packets = fileGen->generatePackets(10);
        savePacketsToFile(packets, "test_packets.bin");
        
        std::cout << "\nAll tests completed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}