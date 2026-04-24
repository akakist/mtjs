#include <iostream>
#include "blst_cp.h"
#include <openssl/rand.h>
#include <nlohmann/json.hpp>
#include "base62.h"
std::vector<unsigned char> randomBytes(size_t len) {
    std::vector<unsigned char> buf(len);
    if (RAND_bytes(buf.data(), static_cast<int>(len)) != 1) {
        throw std::runtime_error("RAND_bytes failed");
    }
    return buf;
}

#include <vector>
#include <string>

#include <sodium.h>
#include <vector>
#include <string>
#include <stdexcept>

struct Ed25519Keypair {
    std::vector<uint8_t> sk_bin; // 64 bytes (seed+expanded or full secret)
    std::vector<uint8_t> pk_bin; // 32 bytes
    // std::string sk_hex;
    // std::string pk_hex;
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
static std::string toHex(const std::string &str) {
    static const char* hex = "0123456789abcdef";
    std::string out; out.resize(str.size() * 2);
    for (size_t i = 0; i < str.size(); ++i) {
        out[2*i]   = hex[(str[i] >> 4) & 0xF];
        out[2*i+1] = hex[str[i] & 0xF];
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

    // kp.sk_hex = toHex(kp.sk_bin.data(), kp.sk_bin.size());
    // kp.pk_hex = toHex(kp.pk_bin.data(), kp.pk_bin.size());
    // printf("ed sk %s\ned pk %s",kp.sk_hex.c_str(),kp.pk_hex.c_str());
    return kp;
}


int main() {
    // Инициализация (BN254 или BLS12-381 в зависимости от сборки)
// 
    // Генерация секретного ключа
    auto buf = randomBytes(32);
    blst_cpp::SecretKey sk(buf.data(), buf.size());
    // sk.setLittleEndian(buf.data(), buf.size()); // задать секретный ключ вручную
    // sk.setByCSPRNG();  // криптографически стойкий случайный скаляр

    // Получение публичного ключа
    blst_cpp::PublicKey pk(sk);
    // sk.getPublicKey(pk);

    // Сообщение
    std::string msg = "Hello Sergey";

    // Подпись
    blst_cpp::Signature sig;
    sig.sign(sk , msg);

    // Проверка
    if (sig.verify(pk, msg)) {
        std::cout << "Signature verified!" << std::endl;
    } else {
        std::cout << "Verification failed!" << std::endl;
    }
    nlohmann::json j;
    j["bls"]["sk"]=base62::encode(sk.serialize());
    j["bls"]["pk"]=base62::encode(pk.serialize());

        // std::cout << "SK:"<< sk.serializeToHexStr() << std::endl;
        // std::cout << "PK:"<< pk.serializeToHexStr() << std::endl;
        // std::cout << "SIG:"<< sig.serializeToHexStr() << std::endl;

    auto kp=generateEd25519Keypair();
    j["ed"]["sk"]=base62::encode(kp.sk_bin);
    j["ed"]["pk"]=base62::encode(kp.pk_bin);
    std::cout<< j.dump(4);

    std::vector<std::string> names={"main","root","n0","n1","n2","n3","n4"};

    for(auto &n:names)
    {
        auto buf = randomBytes(32);
        blst_cpp::SecretKey sk(buf.data(), buf.size());
        blst_cpp::PublicKey pk(sk);

        printf("export k_%s_bls_sk=%s\n",n.c_str(),base62::encode(sk.serialize()).c_str());
        printf("export k_%s_bls_pk=%s\n",n.c_str(),base62::encode(pk.serialize()).c_str());
        auto kp=generateEd25519Keypair();
        printf("export k_%s_ed_sk=%s\n",n.c_str(),base62::encode(kp.sk_bin).c_str());
        printf("export k_%s_ed_pk=%s\n",n.c_str(),base62::encode(kp.pk_bin).c_str());


    }

    return 0;
}
