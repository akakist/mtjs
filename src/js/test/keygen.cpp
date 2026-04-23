#include <iostream>
#include <bls/bls384_256.h>
#include <bls/bls.hpp>
#include <bls/msg.hpp>
#include <mcl/fp.hpp>
#include <openssl/rand.h>
#include <nlohmann/json.hpp>
std::vector<unsigned char> randomBytes(size_t len) {
    std::vector<unsigned char> buf(len);
    if (RAND_bytes(buf.data(), static_cast<int>(len)) != 1) {
        throw std::runtime_error("RAND_bytes failed");
    }
    return buf;
}

#include <bls/bls.hpp>
#include <vector>
#include <string>

#include <sodium.h>
#include <vector>
#include <string>
#include <stdexcept>

struct Ed25519Keypair {
    std::vector<uint8_t> sk_bin; // 64 bytes (seed+expanded or full secret)
    std::vector<uint8_t> pk_bin; // 32 bytes
    std::string sk_hex;
    std::string pk_hex;
};

static std::string toHex(const uint8_t* data, size_t len) {
    static const char* hex = "0123456789abcdef";
    std::string out; out.resize(len * 2);
    for (size_t i = 0; i < len; ++i) {
        out[2*i]   = hex[(data[i] >> 4) & 0xF];
        out[2*i+1] = hex[data[i] & 0xF];
    }
    return out;
}

Ed25519Keypair generateEd25519Keypair() {
    if (sodium_init() < 0) {
        throw std::runtime_error("sodium_init failed");
    }

    Ed25519Keypair kp;
    kp.sk_bin.resize(crypto_sign_SECRETKEYBYTES); // 64
    kp.pk_bin.resize(crypto_sign_PUBLICKEYBYTES); // 32

    if (crypto_sign_keypair(kp.pk_bin.data(), kp.sk_bin.data()) != 0) {
        throw std::runtime_error("crypto_sign_keypair failed");
    }

    kp.sk_hex = toHex(kp.sk_bin.data(), kp.sk_bin.size());
    kp.pk_hex = toHex(kp.pk_bin.data(), kp.pk_bin.size());
    // printf("ed sk %s\ned pk %s",kp.sk_hex.c_str(),kp.pk_hex.c_str());
    return kp;
}


int main() {
    // Инициализация (BN254 или BLS12-381 в зависимости от сборки)
    bls::init();
// 
    // Генерация секретного ключа
    bls::SecretKey sk;
    auto buf = randomBytes(32);
    sk.setLittleEndian(buf.data(), buf.size()); // задать секретный ключ вручную
    // sk.setByCSPRNG();  // криптографически стойкий случайный скаляр

    // Получение публичного ключа
    bls::PublicKey pk;
    sk.getPublicKey(pk);

    // Сообщение
    std::string msg = "Hello Sergey";

    // Подпись
    bls::Signature sig;
    sk.sign(sig, msg);

    // Проверка
    if (sig.verify(pk, msg)) {
        std::cout << "Signature verified!" << std::endl;
    } else {
        std::cout << "Verification failed!" << std::endl;
    }
    nlohmann::json j;
    j["bls"]["sk"]=sk.serializeToHexStr();
    j["bls"]["pk"]=pk.serializeToHexStr();

        // std::cout << "SK:"<< sk.serializeToHexStr() << std::endl;
        // std::cout << "PK:"<< pk.serializeToHexStr() << std::endl;
        // std::cout << "SIG:"<< sig.serializeToHexStr() << std::endl;

    auto kp=generateEd25519Keypair();
    j["ed"]["sk"]=kp.sk_hex;
    j["ed"]["pk"]=kp.pk_hex;
    std::cout<< j.dump(4);

    
    return 0;
}
