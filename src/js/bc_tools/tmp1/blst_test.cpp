// example.cpp
#include "blst_wrapper.h"
#include <iostream>
#include <iomanip>

int main() {
    try {
        // 1. Генерация ключей
        std::string entropy = "0123456789abcdef0123456789abcdef";
        auto [sk, pk] = blst::BlstCrypto::generate_keypair(
            reinterpret_cast<const uint8_t*>(entropy.data()), 
            entropy.size()
        );
        
        std::cout << "Secret key: ";
        for (auto b : sk) std::cout << std::hex << std::setw(2) << (int)b;
        std::cout << std::endl;
        
        // 2. Подпись сообщения
        std::vector<uint8_t> message = {0x48, 0x65, 0x6c, 0x6c, 0x6f}; // "Hello"
        auto sig = blst::BlstCrypto::sign(sk, message);
        
        // 3. Верификация
        bool valid = blst::BlstCrypto::verify(pk, sig, message);
        std::cout << "Signature valid: " << std::boolalpha << valid << std::endl;
        
        // 4. Агрегация нескольких подписей
        auto [sk2, pk2] = blst::BlstCrypto::generate_keypair(
            reinterpret_cast<const uint8_t*>("another entropy12345"), 20
        );
        
        std::vector<blst::PublicKey> keys = {pk, pk2};
        std::vector<std::vector<uint8_t>> msgs = {
            {0x48, 0x65, 0x6c, 0x6c, 0x6f},
            {0x57, 0x6f, 0x72, 0x6c, 0x64}
        };
        
        auto sig1 = blst::BlstCrypto::sign(sk, msgs[0]);
        auto sig2 = blst::BlstCrypto::sign(sk2, msgs[1]);
        
        auto agg_sig = blst::BlstCrypto::aggregate_signatures({sig1, sig2});
        bool agg_valid = blst::BlstCrypto::aggregate_verify(keys, msgs, agg_sig);
        
        std::cout << "Aggregate signature valid: " << agg_valid << std::endl;
        
    } catch (const blst::BlstException& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}