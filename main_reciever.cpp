#include "BasicReceiver.h"

int main() {
    BasicReceiver receiver;
    
    if (!receiver.initialize(8888)) {
        std::cout << "Failed to initialize receiver!" << std::endl;
        return 1;
    }
    
    std::cout << "Waiting for packets..." << std::endl;
    
    while (true) {
        auto data = receiver.receiveData();
        
        if (!data.empty()) {
            std::cout << "Got data! Size: " << data.size() << " bytes" << std::endl;
            
            // Просто выводим что получили
            std::cout << "Data: ";
            for (auto byte : data) {
                printf("%02X ", byte);
            }
            std::cout << std::endl << std::endl;
        }
    }
    
    return 0;
}