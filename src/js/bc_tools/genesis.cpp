#include "getenv2.h"
#include "IDatabase.h"
#include "normalizeNick.h"
#include "Key.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include "cells.h"

// addr (hex, 64) / nick → ADDRESS_id
static ADDRESS_id resolve_addr(const std::string& s) {
    ADDRESS_id a;
    if (s.size() == 64) {
        a.addr = base16::decode(s);   // hex
        if (a.addr.size() != 32)
            throw CommonError("genesis: hex addr must be 32 bytes");
    } else {
        std::string norm = normalizeNick(s);
        if (norm.empty()) throw CommonError("genesis: bad nick '%s'", s.c_str());
        a.addr = blake2b_hash(norm).container;
    }
    return a;
}

static ADDRESS_id resolve_user(const nlohmann::json& u) {
    if (u.contains("addr") && u["addr"].is_string())
        return resolve_addr(u["addr"].get<std::string>());
    if (u.contains("nick") && u["nick"].is_string())
        return resolve_addr(u["nick"].get<std::string>());
    throw CommonError("genesis: user must have 'addr' or 'nick'");
}

static std::string user_label(const nlohmann::json& u) {
    if (u.contains("addr") && u["addr"].is_string()) return u["addr"].get<std::string>();
    if (u.contains("nick") && u["nick"].is_string()) return u["nick"].get<std::string>();
    return "<unknown>";
}
THASH_id proceed_merkle_on_transaction_pool_hashers(const REF_getter<Cellable> &r, _db_to_save &db_to_save_Z);

void init_genesis(Cellable* root, IDatabase* db) 
{
    MUTEX_INSPECTOR;
    if(!root)
        throw CommonError("[init_genesis] if(!root)");
    auto st = cells::getStateNoCreate(root, db);
    if (st.valid()) return;

    logErr2("RUN GENESIS");

    const char* env_path = getenv("MTJS_GENESIS_PATH");
    std::string path = env_path ? env_path : "genesis.json";
    std::ifstream f(path);
    if (!f) throw CommonError("genesis: cannot open %s", path.c_str());
    nlohmann::json j;
    try { f >> j; }
    catch (const std::exception& e) { throw CommonError("genesis: parse %s: %s", path.c_str(), e.what()); }

    // ▼▼▼ INSERT: genesis_time (для activity)
    int64_t genesis_time = j.value("genesis_time", (int64_t)time(NULL));
    // ▲▲▲

    auto nl=cells::getNodeListOrCreate(root,db,NULL);
    nl->setDirty(NULL);
    auto votes=cells::getVotesOrCreate(root,db,NULL);
    votes->setDirty(NULL);

    auto st2 = cells::getStateOrCreate(root,db,NULL);
    st2->chain_id      = j.value("chain_id", (int64_t)1);
    st2->genesis_time  = j.value("genesis_time", (int64_t)time(NULL));
    st2->total_supply  = i128(0);
    st2->total_stake   = i128(0);
    st2->account_count = 0;
    st2->node_count    = 0;
    st2->contract_count = 0;
    st2->tx_count      = 0;
    st2->block_height  = 0;
    st2->setDirty(NULL);

    auto v = cells::getValuesOrCreate(root, db, NULL);
    if (j.contains("params") && j["params"].is_object()) {
        for (auto& [k, val] : j["params"].items()) {
            if (!val.is_number_integer())
                throw CommonError("genesis: params.%s must be integer", k.c_str());
            v->values[k] = val.get<int64_t>();
        }
    }
    if (j.contains("emitters") && j["emitters"].is_array()) {
        for (auto& addr_hex : j["emitters"]) {
            if (!addr_hex.is_string()) continue;
            ADDRESS_id addr;
            addr.addr = base16::decode(addr_hex.get<std::string>());
            if (addr.addr.size() != 32)
                throw CommonError("genesis: emitters addr must be 32 bytes hex (got %zu)", addr.addr.size());
            v->emitters_bin.insert(addr);
        }
    }
    v->setDirty(NULL);

    if (j.contains("users") && j["users"].is_array()) {
        for (auto& u : j["users"]) {
            std::string label = user_label(u);
            ADDRESS_id addr = resolve_user(u);

            auto ua = cells::getAddressStateOrCreate(root, db,addr, NULL);
            int64_t bal = 0;
            if (u.contains("balance") && u["balance"].is_string())
                bal = std::stoll(u["balance"].get<std::string>());
            {  ua->balance = bal; }

            if (u.contains("keys") && u["keys"].is_array()) {
                auto& keys = u["keys"];
                if (keys.size() == 0 || keys.size() > MAX_KEYS_PER_ADDRESS)
                    throw CommonError("genesis: users.%s keys 1..5 required", label.c_str());
                for (auto& k : keys) {
                    if (!k.contains("type") || !k.contains("keybuffer"))
                        throw CommonError("genesis: users.%s key {type,keybuffer} required", label.c_str());
                    uint8_t type = (uint8_t)k["type"].get<int>();
                    auto kb = base16::decode(k["keybuffer"].get<std::string>());
                    if (type == KEY_ED25519 && kb.size() != 32)
                        throw CommonError("genesis: users.%s bad ed25519 size", label.c_str());
                    if ((type == KEY_SECP256K1 || type == KEY_P256) && kb.size() != 33 && kb.size() != 65)
                        throw CommonError("genesis: users.%s bad ecdsa size", label.c_str());
                    auto id = ua->addKey(type, kb);
                    if (!id) throw CommonError("genesis: users.%s addKey failed", label.c_str());
                }
            }
            ua->setDirty(NULL);

            // ▼▼▼ INSERT: activity для адреса (ttl = 0, вечный)
            cells::getActivityOrCreate(root, db, ua->getDbId(), {}, 0, genesis_time, NULL);
            // ▲▲▲

            st2->total_supply += i128(bal);
        }
    }
    st2->setDirty(NULL);

    if (j.contains("nodes") && j["nodes"].is_array()) {
        for (auto& n : j["nodes"]) {
            if (!n.contains("name") || !n["name"].is_string())
                throw CommonError("genesis: node.name required");
            NODE_id name; name.container = n["name"].get<std::string>();
            if (cells::getNodeNoCreate(root,db, name).valid())
                throw CommonError("genesis: duplicate node %s", name.container.c_str());

            if (!n.contains("owner") || !n["owner"].is_string())
                throw CommonError("genesis: node.%s owner required", name.container.c_str());
            ADDRESS_id owner = resolve_addr(n["owner"].get<std::string>());

            auto nn = cells::addNode(root,db,name, NULL);

            blst_cpp::PublicKey bls_pk;
            if (!n.contains("bls_pk") || !n["bls_pk"].is_string())
                throw CommonError("genesis: node.%s bls_pk required", name.container.c_str());
            bls_pk.deserializeHexStr(n["bls_pk"].get<std::string>());

            if (!n.contains("ed_pk") || !n["ed_pk"].is_string())
                throw CommonError("genesis: node.%s ed_pk required", name.container.c_str());
            auto ed_pk = base16::decode(n["ed_pk"].get<std::string>());

            std::string ip;
            if (n.contains("ip") && n["ip"].is_string()) ip = n["ip"].get<std::string>();

            nn->init(name, owner, bls_pk, ed_pk, ip);

            int64_t stake = 0;
            if (n.contains("stake") && n["stake"].is_string())
                stake = std::stoll(n["stake"].get<std::string>());
            nn->add_stake(owner, stake);
            nn->setDirty(NULL);

            // ▼▼▼ INSERT: activity для ноды (ttl = 0, вечный)
            cells::getActivityOrCreate(root, db, nn->getDbId(), {}, 0, genesis_time, NULL);
            // ▲▲▲

            st2->total_stake += i128(stake);
        }
    }
    st2->setDirty(NULL);
    _db_to_save dbz;
    proceed_merkle_on_transaction_pool_hashers(root,dbz);
    db->write_granules_batch(dbz);
    logErr2("write_granules_batch %d granules",dbz.cells.size());
}

