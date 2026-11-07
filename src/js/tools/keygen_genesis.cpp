// tools/keygen_genesis.cpp
// Генерит:
//   genesis.json   — публичный (users{nick|addr, balance, keys}, emitters, nodes, params)
//   keys_test.json — приватный (addr, sk, bls_sk, ed_sk) — НЕ в git (мимо репы)
//
// users:
//   root               — nick, 1 ed25519
//   alice..(n_users-2) — nick, 1..4 keys (alice 4: 2 ed, secp, p256)
//   u0..u19            — addr = blake2b(normalizeNick("u<i>")) hex, 1 ed25519
//                        (исключая u16/u19 — normalizeNick схлопывает 6→G, 9→G → UIG дубль)
//
// emitters = все users (первый ключ каждого) — mint разрешён всем
//
// keys_test.json users[name] = { addr (hex), keys: [{type, sk, pk}] }
//   addr нужен тестам (b_test-node-pause) — чтобы использовать hex, не addr_from_name
//   (u16/u19 через addr_from_name дают один hex → verify failed)

#define OPENSSL_SUPPRESS_DEPRECATED

#include <sodium.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#include <blst.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <set>
#include "normalizeNick.h"
#include "blake2bHasher.h"

// ---------- hex ----------
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
struct EdKey { std::string sk_bin, pk_bin; };
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
struct EcKey { std::string sk_bin, pk_bin; };   // sk 32, pk 33 compressed
static EcKey gen_ecdsa(int nid) {
    EC_KEY* ec = EC_KEY_new_by_curve_name(nid);
    if (!ec) throw std::runtime_error("EC_KEY_new_by_curve_name failed");
    if (!EC_KEY_generate_key(ec)) { EC_KEY_free(ec); throw std::runtime_error("EC_KEY_generate_key failed"); }

    const BIGNUM* priv = EC_KEY_get0_private_key(ec);
    std::string sk(32, '\0');
    BN_bn2binpad(priv, (unsigned char*)&sk[0], 32);

    const EC_GROUP* group = EC_KEY_get0_group(ec);
    const EC_POINT* pub   = EC_KEY_get0_public_key(ec);
    size_t pklen = EC_POINT_point2oct(group, pub, POINT_CONVERSION_COMPRESSED, nullptr, 0, nullptr);
    std::string pk(pklen, '\0');
    EC_POINT_point2oct(group, pub, POINT_CONVERSION_COMPRESSED, (unsigned char*)&pk[0], pklen, nullptr);

    EC_KEY_free(ec);
    return {sk, pk};
}

// ---------- BLS (blst) ----------
struct BlsKey { std::string sk_bin, pk_bin; };   // sk 32, pk 48
static BlsKey gen_bls() {
    unsigned char ikm[32];
    randombytes_buf(ikm, sizeof(ikm));
    blst_scalar sk;
    blst_keygen(&sk, ikm, sizeof(ikm), nullptr, 0);
    blst_p1 pk;
    blst_sk_to_pk_in_g1(&pk, &sk);

    BlsKey k;
    k.sk_bin.resize(32);
    memcpy(&k.sk_bin[0], sk.b, 32);
    k.pk_bin.resize(48);
    blst_p1_compress((unsigned char*)&k.pk_bin[0], &pk);
    return k;
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

// blake2b(normalizeNick(nick)) — hex
static std::string addr_from_nick(const std::string& nick) {
    std::string norm = normalizeNick(nick);
    if (norm.empty())
        throw std::runtime_error("bad nick: " + nick);
    return hex(blake2b_hash(norm).container);
}

int main(int argc, char** argv) {
    if (sodium_init() < 0) { std::cerr << "sodium_init failed\n"; return 1; }

    int n_users = 5;                 // root + (n_users-1) nick-users (alice..)
    int n_nodes = 10;
    std::string out_genesis = "genesis.json";
    std::string out_keys    = "keys_test.json";

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&](const char* name) -> std::string {
            if (i + 1 >= argc) throw std::runtime_error(std::string("missing value for ") + name);
            return argv[++i];
        };
        if (a == "--users") n_users = std::atoi(next("--users").c_str());
        else if (a == "--nodes") n_nodes = std::atoi(next("--nodes").c_str());
        else if (a == "--out-genesis") out_genesis = next("--out-genesis");
        else if (a == "--out-keys") out_keys = next("--out-keys");
        else if (a == "--help" || a == "-h") {
            std::cout << "usage: " << argv[0]
                      << " [--users N] [--nodes M] [--out-genesis P] [--out-keys P]\n";
            return 0;
        }
        else { std::cerr << "unknown: " << a << "\n"; return 1; }
    }
    if (n_users <= 1 || n_users > 16) { std::cerr << "--users 2..16 (root + nick-users)\n"; return 1; }
    if (n_nodes <= 0 || n_nodes > 16) { std::cerr << "--nodes 1..16\n"; return 1; }

    // ============================================================
    // users + keys
    // ============================================================
    nlohmann::json g_users = nlohmann::json::array();
    nlohmann::json k_users = nlohmann::json::object();

    std::set<std::string> addr_set;   // уникальность addr

    auto push_user = [&](const nlohmann::json& u, const std::string& addr_hex, const std::string &name) {
        if (!addr_set.insert(addr_hex).second)
            throw std::runtime_error("duplicate addr: " + addr_hex +" "+name);
        g_users.push_back(u);
    };

    // 1) root (nick, ed25519, 1 key)
    {
        auto ed = gen_ed25519();
        std::string addr = addr_from_nick("root");
        nlohmann::json u;
        u["nick"] = "root";
        u["balance"] = "100000000";
        u["keys"] = nlohmann::json::array({ { {"type",0}, {"keybuffer", hex(ed.pk_bin)} } });
        push_user(u, addr,"root");

        k_users["root"]["addr"] = addr;
        k_users["root"]["keys"] = nlohmann::json::array({
            { {"type",0}, {"sk", hex(ed.sk_bin)}, {"pk", hex(ed.pk_bin)} }
        });
    }

    // 2) alice.. — nick; alice 4 keys (2 ed, secp, p256); остальные 1 ed
    for (int i = 0; i < n_users - 1; ++i) {
        std::string name = user_name(i);
        std::string addr = addr_from_nick(name);
        nlohmann::json u;
        u["nick"] = name;
        u["balance"] = "10000000";

        nlohmann::json gkeys = nlohmann::json::array();
        nlohmann::json kkeys = nlohmann::json::array();

        if (i == 0) {   // alice — 4 keys
            auto ed1 = gen_ed25519();
            auto ed2 = gen_ed25519();
            auto ec1 = gen_ecdsa(NID_secp256k1);
            auto ec2 = gen_ecdsa(NID_X9_62_prime256v1);
            gkeys.push_back({ {"type",0}, {"keybuffer", hex(ed1.pk_bin)} });
            gkeys.push_back({ {"type",0}, {"keybuffer", hex(ed2.pk_bin)} });
            gkeys.push_back({ {"type",1}, {"keybuffer", hex(ec1.pk_bin)} });
            gkeys.push_back({ {"type",2}, {"keybuffer", hex(ec2.pk_bin)} });
            kkeys.push_back({ {"type",0}, {"sk", hex(ed1.sk_bin)}, {"pk", hex(ed1.pk_bin)} });
            kkeys.push_back({ {"type",0}, {"sk", hex(ed2.sk_bin)}, {"pk", hex(ed2.pk_bin)} });
            kkeys.push_back({ {"type",1}, {"sk", hex(ec1.sk_bin)}, {"pk", hex(ec1.pk_bin)} });
            kkeys.push_back({ {"type",2}, {"sk", hex(ec2.sk_bin)}, {"pk", hex(ec2.pk_bin)} });
        } else {         // остальные — 1 ed25519
            auto ed = gen_ed25519();
            gkeys.push_back({ {"type",0}, {"keybuffer", hex(ed.pk_bin)} });
            kkeys.push_back({ {"type",0}, {"sk", hex(ed.sk_bin)}, {"pk", hex(ed.pk_bin)} });
        }
        u["keys"] = gkeys;
        push_user(u, addr, name);
        k_users[name]["addr"] = addr;
        k_users[name]["keys"] = kkeys;
    }

    // 3) u0..u19 — addr = blake2b(normalizeNick("u<i>")) hex; 1 ed25519
    //    ВАЖНО: u16/u19 normalizeNick схлопывает (6→G, 9→G → UIG) — пропускаем u16/u19,
    //    добавляем u20/u21 вместо них (U20/U21 — уникальны).
    {
        std::vector<int> u_idx;
        for (int i = 0; i < 20; ++i) {
            if (i == 16 || i == 19 || i == 6 || i == 9) continue;   // UIG дубль
            u_idx.push_back(i);
        }
        u_idx.push_back(20);
        u_idx.push_back(21);
        u_idx.push_back(22);
        u_idx.push_back(23);

        for (int i : u_idx) {
            auto ed = gen_ed25519();
            std::string name = "u" + std::to_string(i);
            std::string addr = addr_from_nick(name);

            nlohmann::json u;
            u["addr"] = addr;
            u["balance"] = "10000000";
            u["keys"] = nlohmann::json::array({
                { {"type",0}, {"keybuffer", hex(ed.pk_bin)} }
            });
            push_user(u, addr, name);

            k_users[name]["addr"] = addr;
            k_users[name]["keys"] = nlohmann::json::array({
                { {"type",0}, {"sk", hex(ed.sk_bin)}, {"pk", hex(ed.pk_bin)} }
            });
        }
    }

    // ============================================================
    // nodes n0..n{n_nodes-1}
    // ============================================================
    nlohmann::json g_nodes = nlohmann::json::array();
    nlohmann::json k_nodes = nlohmann::json::object();
    for (int i = 0; i < n_nodes; ++i) {
        auto bls = gen_bls();
        auto ed  = gen_ed25519();
        std::string name = "n" + std::to_string(i);

        nlohmann::json n;
        n["name"] = name;
        n["owner"] = "root";
        n["bls_pk"] = hex(bls.pk_bin);
        n["ed_pk"] = hex(ed.pk_bin);
        n["ip"] = "127.0.0.1:" + std::to_string(2300 + i);
        n["stake"] = "100";
        n["description"] = "genesis node " + name;
        g_nodes.push_back(n);

        k_nodes[name]["bls_sk"] = hex(bls.sk_bin);
        k_nodes[name]["bls_pk"] = hex(bls.pk_bin);
        k_nodes[name]["ed_sk"]  = hex(ed.sk_bin);
        k_nodes[name]["ed_pk"]  = hex(ed.pk_bin);
    }

    // ============================================================
    // genesis.json (public)
    // ============================================================
    nlohmann::json genesis;
    genesis["chain_id"] = 1;
    genesis["genesis_time"] = (int64_t)time(NULL);
    genesis["params"] = {
        {"gas_per_token", 1000},
        {"gas_per_byte", 1},
        {"gas_per_jsi_step", 10},
        {"gas_per_hostcall", 500},
        {"gas_per_call", 1000},
        {"min_tx_fee", 1},
        {"gas_contract_deploy", 10000},
        {"gas_create_base", 100},
        {"ttl_seconds_per_gas", 86400},
        {"gas_create_address", 1000},
        {"validator_count_percent", 50},
        {"validator_min_stake", 50},
        {"missed_rounds_max", 100},
        {"answered_committee_min", 10},
        {"ttl_cache", 20},
    };

    // emitters = addr (hex) всех users
    {
        nlohmann::json emitters = nlohmann::json::array();
        for (auto& u : g_users) {
            if (u.contains("nick") && u["nick"].is_string()) {
                emitters.push_back(addr_from_nick(u["nick"].get<std::string>()));
            } else if (u.contains("addr") && u["addr"].is_string()) {
                emitters.push_back(u["addr"].get<std::string>());
            }
        }
        genesis["emitters"] = emitters;
    }
    genesis["users"] = g_users;
    genesis["nodes"] = g_nodes;

    // ============================================================
    // keys_test.json (PRIVATE) — users[name] = { addr, keys: [{type, sk, pk}] }
    // ============================================================
    nlohmann::json keys;
    keys["users"] = k_users;
    keys["nodes"] = k_nodes;

    // ============================================================
    // write
    // ============================================================
    {
        std::ofstream f(out_genesis);
        if (!f) { std::cerr << "cannot write " << out_genesis << "\n"; return 1; }
        f << genesis.dump(2) << "\n";
    }
    {
        std::ofstream f(out_keys);
        if (!f) { std::cerr << "cannot write " << out_keys << "\n"; return 1; }
        f << keys.dump(2) << "\n";
    }

    std::cerr << "wrote " << out_genesis
              << " (users{nick|addr,balance,keys}, emitters=all users, nodes)\n";
    std::cerr << "wrote " << out_keys
              << " (PRIVATE — keep out of git! users[name]={addr,keys:[{type,sk,pk}]})\n";
    return 0;
}

