// tools/keygen_users.cpp
// Генерация keys_users.json: { "alice": { "keys": [ {type, sk, pk}, ... ] }, ... }
// Ed25519 — libsodium (64 sk, 32 pk).
// secp256k1/p256 — OpenSSL (32 sk raw, 33 pk compressed).
//
// Сборка (CMake): уже в проекте (tools/CMakeLists.txt — keygen_users)
// Запуск: ./keygen_users <count> [out.json]

// подавляем deprecated warnings OpenSSL 3.0 (EC_KEY_* — legacy, но работают)
#define OPENSSL_SUPPRESS_DEPRECATED

#include <sodium.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>

static std::string hex(const unsigned char* p, size_t n) {
    static const char* h = "0123456789abcdef";
    std::string s; s.resize(n*2);
    for (size_t i = 0; i < n; ++i) { s[2*i]=h[p[i]>>4]; s[2*i+1]=h[p[i]&0xF]; }
    return s;
}
static std::string hex(const std::string& s) {
    return hex((const unsigned char*)s.data(), s.size());
}

// ---------- Ed25519 (libsodium) ----------
struct EdKey { std::string sk_bin; std::string pk_bin; };
static EdKey gen_ed25519() {
    EdKey k;
    k.pk_bin.resize(crypto_sign_PUBLICKEYBYTES);
    k.sk_bin.resize(crypto_sign_SECRETKEYBYTES);
    if (crypto_sign_keypair((unsigned char*)k.pk_bin.data(),
                            (unsigned char*)k.sk_bin.data()) != 0)
        throw std::runtime_error("crypto_sign_keypair failed");
    return k;
}

// ---------- ECDSA (OpenSSL) ----------
struct EcKey { std::string sk_bin; std::string pk_bin; };   // sk 32 raw, pk 33 compressed
static EcKey gen_ecdsa(int nid) {
    EC_KEY* ec = EC_KEY_new_by_curve_name(nid);
    if (!ec) throw std::runtime_error("EC_KEY_new_by_curve_name failed");
    if (!EC_KEY_generate_key(ec)) { EC_KEY_free(ec); throw std::runtime_error("EC_KEY_generate_key failed"); }

    // sk: 32 байта (BN)
    const BIGNUM* priv = EC_KEY_get0_private_key(ec);
    std::string sk(32, '\0');
    BN_bn2binpad(priv, (unsigned char*)&sk[0], 32);

    // pk: compressed (33) — EC_POINT_point2oct нужен EC_GROUP*
    const EC_GROUP* group = EC_KEY_get0_group(ec);
    const EC_POINT* pub   = EC_KEY_get0_public_key(ec);
    size_t pklen = EC_POINT_point2oct(group, pub, POINT_CONVERSION_COMPRESSED, nullptr, 0, nullptr);
    std::string pk(pklen, '\0');
    EC_POINT_point2oct(group, pub, POINT_CONVERSION_COMPRESSED, (unsigned char*)&pk[0], pklen, nullptr);

    EC_KEY_free(ec);
    return {sk, pk};
}

// ---------- имена ----------
static std::string user_name(int i) {
    static const char* names[] = {
        "alice","bob","carol","dave","erin","frank","grace","heidi",
        "ivan","judy","karl","linda","mallory","niaj","olivia","peggy"
    };
    if (i < (int)(sizeof(names)/sizeof(names[0]))) return names[i];
    return "user" + std::to_string(i);
}

int main(int argc, char** argv) {
    if (sodium_init() < 0) { std::cerr << "sodium_init failed\n"; return 1; }
    int count = (argc >= 2) ? std::atoi(argv[1]) : 5;
    std::string out_path = (argc >= 3) ? argv[2] : "keys_users.json";
    if (count <= 0 || count > 16) { std::cerr << "count 1..16\n"; return 1; }

    nlohmann::json j;
    for (int i = 0; i < count; ++i) {
        std::string name = user_name(i);
        auto ed1 = gen_ed25519();
        auto ed2 = gen_ed25519();
        auto ec1 = gen_ecdsa(NID_secp256k1);
        auto ec2 = gen_ecdsa(NID_X9_62_prime256v1);

        j[name]["keys"] = nlohmann::json::array({
            { {"type", 0}, {"sk", hex(ed1.sk_bin)}, {"pk", hex(ed1.pk_bin)} },   // ed25519 #0
            { {"type", 0}, {"sk", hex(ed2.sk_bin)}, {"pk", hex(ed2.pk_bin)} },   // ed25519 #1
            { {"type", 1}, {"sk", hex(ec1.sk_bin)}, {"pk", hex(ec1.pk_bin)} },   // secp256k1
            { {"type", 2}, {"sk", hex(ec2.sk_bin)}, {"pk", hex(ec2.pk_bin)} },   // p256
        });
    }
    std::ofstream f(out_path);
    if (!f) { std::cerr << "cannot write " << out_path << "\n"; return 1; }
    f << j.dump(2) << "\n";
    std::cerr << "wrote " << count << " users -> " << out_path << "\n";
    return 0;
}