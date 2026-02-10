#pragma once
#include <vector>
#include <cstdint>
#include <memory>
#include <map>
#include <random>
#include <string>
#include <algorithm>

// === ������� ���������� ===
class ILengthGenerator {
public:
    virtual ~ILengthGenerator() = default;
    virtual uint32_t generateLength() = 0;
    virtual std::map<std::string, std::string> getParameters() const = 0;
};

class IContentGenerator {
public:
    virtual ~IContentGenerator() = default;
    virtual std::vector<uint8_t> generateContent(uint32_t length) = 0;
    virtual std::map<std::string, std::string> getParameters() const = 0;
};

// === ���������� ��� ���������� ����������� ===
class IFixedLengthGenerator : public ILengthGenerator {
public:
    virtual void setLength(uint32_t length) = 0;
    virtual uint32_t getLength() const = 0;
};

class IRandomLengthGenerator : public ILengthGenerator {
public:
    virtual void setRange(uint32_t min, uint32_t max) = 0;
    virtual uint32_t getMin() const = 0;
    virtual uint32_t getMax() const = 0;
};

class ISequentialLengthGenerator : public ILengthGenerator {
public:
    virtual void resetSequence() = 0;
    virtual uint32_t getCurrentPosition() const = 0;
};

// === ���������� ����������� ���� ===
class FixedLengthGenerator : public IFixedLengthGenerator {
private:
    uint32_t _fixedLength;
public:
    FixedLengthGenerator(uint32_t length) : _fixedLength(length) {}

    uint32_t generateLength() override {
        return _fixedLength;
    }

    std::map<std::string, std::string> getParameters() const override {
        return {
            {"type", "fixed"},
            {"length", std::to_string(_fixedLength)}
        };
    }

    void setLength(uint32_t length) override {
        _fixedLength = length;
    }

    uint32_t getLength() const override {
        return _fixedLength;
    }
};

class UniformLengthGenerator : public IRandomLengthGenerator {
private:
    uint32_t _minLength;
    uint32_t _maxLength;
    std::mt19937 _generator;
    std::uniform_int_distribution<uint32_t> _dist;

public:
    UniformLengthGenerator(uint32_t minLength, uint32_t maxLength)
        : _minLength(minLength), _maxLength(maxLength),
        _dist(_minLength, maxLength) {
        _generator.seed(std::random_device{}());
    }

    uint32_t generateLength() override {
        return _dist(_generator);
    }

    std::map<std::string, std::string> getParameters() const override {
        return {
            {"type", "uniform"},
            {"min", std::to_string(_minLength)},
            {"max", std::to_string(_maxLength)}
        };
    }

    void setRange(uint32_t min, uint32_t max) override {
        _minLength = min;
        _maxLength = max;
        _dist = std::uniform_int_distribution<uint32_t>(min, max);
    }

    uint32_t getMin() const override { return _minLength; }
    uint32_t getMax() const override { return _maxLength; }
};

class SequentialLengthGenerator : public ISequentialLengthGenerator {
private:
    std::vector<uint32_t> _sequence;
    size_t _currentIndex;

public:
    SequentialLengthGenerator(const std::vector<uint32_t>& sequence)
        : _sequence(sequence), _currentIndex(0) {}

    uint32_t generateLength() override {
        if (_sequence.empty()) return 0;
        uint32_t length = _sequence[_currentIndex];
        _currentIndex = (_currentIndex + 1) % _sequence.size();
        return length;
    }

    std::map<std::string, std::string> getParameters() const override {
        return {
            {"type", "sequential"},
            {"sequence_size", std::to_string(_sequence.size())},
            {"current_index", std::to_string(_currentIndex)}
        };
    }

    void resetSequence() override {
        _currentIndex = 0;
    }

    uint32_t getCurrentPosition() const override {
        return _currentIndex;
    }

    void setSequence(const std::vector<uint32_t>& sequence) {
        _sequence = sequence;
        resetSequence();
    }
};

// === ���������� ����������� ����������� ===
class RandomContentGenerator : public IContentGenerator {
private:
    std::mt19937 _generator;
    std::uniform_int_distribution<int> _dist; // int ������ uint8_t

public:
    RandomContentGenerator() : _dist(0, 255) {
        _generator.seed(std::random_device{}());
    }

    std::vector<uint8_t> generateContent(uint32_t length) override {
        std::vector<uint8_t> content(length);
        for (auto& byte : content) {
            byte = static_cast<uint8_t>(_dist(_generator));
        }
        return content;
    }
    std::map<std::string, std::string> getParameters() const override {
        return {
            {"type", "random"},
            {"range", "0-255"}
        };
    }
};

class PatternContentGenerator : public IContentGenerator {
private:
    std::vector<uint8_t> _pattern;

public:
    PatternContentGenerator(const std::vector<uint8_t>& pattern)
        : _pattern(pattern) {}

    std::vector<uint8_t> generateContent(uint32_t length) override {
        std::vector<uint8_t> content(length);
        for (size_t i = 0; i < length; ++i) {
            content[i] = _pattern[i % _pattern.size()];
        }
        return content;
    }

    std::map<std::string, std::string> getParameters() const override {
        return {
            {"type", "pattern"},
            {"pattern_size", std::to_string(_pattern.size())}
        };
    }

    void setPattern(const std::vector<uint8_t>& pattern) {
        _pattern = pattern;
    }
};

class IncrementalContentGenerator : public IContentGenerator {
private:
    uint8_t _currentValue;

public:
    IncrementalContentGenerator(uint8_t startValue = 0)
        : _currentValue(startValue) {}

    std::vector<uint8_t> generateContent(uint32_t length) override {
        std::vector<uint8_t> content(length);
        for (auto& byte : content) {
            byte = _currentValue++;
        }
        return content;
    }

    std::map<std::string, std::string> getParameters() const override {
        return {
            {"type", "incremental"},
            {"current_value", std::to_string(_currentValue)}
        };
    }

    void reset(uint8_t startValue = 0) {
        _currentValue = startValue;
    }
};

class ZerosContentGenerator : public IContentGenerator {
public:
    std::vector<uint8_t> generateContent(uint32_t length) override {
        return std::vector<uint8_t>(length, 0);
    }

    std::map<std::string, std::string> getParameters() const override {
        return {
            {"type", "zeros"}
        };
    }
};

// === �������� ��������� ������� ===
class PacketGenerator {
private:
    std::unique_ptr<ILengthGenerator> _lengthGenerator;
    std::unique_ptr<IContentGenerator> _contentGenerator;
    uint64_t _packetCounter;

public:
    PacketGenerator(std::unique_ptr<ILengthGenerator> lengthGen,
        std::unique_ptr<IContentGenerator> contentGen)
        : _lengthGenerator(std::move(lengthGen))
        , _contentGenerator(std::move(contentGen))
        , _packetCounter(0) {}

    struct GeneratedPacket {
        std::vector<uint8_t> data;
        uint32_t length;
        uint64_t sequenceNumber;
        std::map<std::string, std::string> metadata;
    };

    GeneratedPacket generatePacket() {
        GeneratedPacket packet;
        packet.length = _lengthGenerator->generateLength();
        packet.data = _contentGenerator->generateContent(packet.length);
        packet.sequenceNumber = _packetCounter++;

        // �������� ����������
        auto lengthParams = _lengthGenerator->getParameters();
        auto contentParams = _contentGenerator->getParameters();
        packet.metadata.insert(lengthParams.begin(), lengthParams.end());
        packet.metadata.insert(contentParams.begin(), contentParams.end());
        packet.metadata["sequence_number"] = std::to_string(packet.sequenceNumber);

        return packet;
    }

    std::vector<GeneratedPacket> generatePackets(size_t count) {
        std::vector<GeneratedPacket> packets;
        packets.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            packets.push_back(generatePacket());
        }
        return packets;
    }

    // ������� ��� ������� � �����������
    ILengthGenerator* getLengthGenerator() { return _lengthGenerator.get(); }
    IContentGenerator* getContentGenerator() { return _contentGenerator.get(); }

    uint64_t getPacketsGenerated() const { return _packetCounter; }
    void resetCounter() { _packetCounter = 0; }
};

// === ������� ��� �������� �������� ����������� ===
class GeneratorFactory {
public:
    // ������� ��� ����������� ����
    static std::unique_ptr<FixedLengthGenerator> createFixedLength(uint32_t length) {
        return std::make_unique<FixedLengthGenerator>(length);
    }

    static std::unique_ptr<UniformLengthGenerator> createUniformLength(uint32_t min, uint32_t max) {
        return std::make_unique<UniformLengthGenerator>(min, max);
    }

    static std::unique_ptr<SequentialLengthGenerator> createSequentialLength(
        const std::vector<uint32_t>& sequence) {
        return std::make_unique<SequentialLengthGenerator>(sequence);
    }

    // ������� ��� ����������� �����������
    static std::unique_ptr<RandomContentGenerator> createRandomContent() {
        return std::make_unique<RandomContentGenerator>();
    }

    static std::unique_ptr<PatternContentGenerator> createPatternContent(
        const std::vector<uint8_t>& pattern) {
        return std::make_unique<PatternContentGenerator>(pattern);
    }

    static std::unique_ptr<IncrementalContentGenerator> createIncrementalContent(
        uint8_t startValue = 0) {
        return std::make_unique<IncrementalContentGenerator>(startValue);
    }

    static std::unique_ptr<ZerosContentGenerator> createZerosContent() {
        return std::make_unique<ZerosContentGenerator>();
    }

    // ���������������� ������������ ��� ������������ GSE
    static std::unique_ptr<PacketGenerator> createVoIPGenerator() {
        auto lengthGen = createUniformLength(64, 128);
        auto contentGen = createRandomContent();
        return std::make_unique<PacketGenerator>(std::move(lengthGen), std::move(contentGen));
    }

    static std::unique_ptr<PacketGenerator> createVideoStreamGenerator() {
        auto lengthGen = createUniformLength(500, 1500);
        auto contentGen = createPatternContent({ 0x00, 0xFF, 0xAA, 0x55 });
        return std::make_unique<PacketGenerator>(std::move(lengthGen), std::move(contentGen));
    }

    static std::unique_ptr<PacketGenerator> createBurstyTrafficGenerator() {
        // ������������������: ������ ������, ����� �������
        std::vector<uint32_t> sequence = { 64, 64, 64, 128, 1500, 1500, 64, 64 };
        auto lengthGen = createSequentialLength(sequence);
        auto contentGen = createIncrementalContent();
        return std::make_unique<PacketGenerator>(std::move(lengthGen), std::move(contentGen));
    }

    static std::unique_ptr<PacketGenerator> createMTUBoundaryGenerator() {
        // ������ �� ������� MTU ��� ������������ ������������
        std::vector<uint32_t> sequence = { 1499, 1500, 1501, 1498, 1502 };
        auto lengthGen = createSequentialLength(sequence);
        auto contentGen = createRandomContent();
        return std::make_unique<PacketGenerator>(std::move(lengthGen), std::move(contentGen));
    }
};