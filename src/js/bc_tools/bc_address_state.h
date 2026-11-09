#pragma once
#include "cellable.h"
#include "Key.h"
#include <map>
#include <optional>
#include <set>
#include "CONTRACT_id.h"
#include "NODE_id.h"

static constexpr size_t MAX_KEYS_PER_ADDRESS = 5;

struct bc_address_state: public data_base {
    bc_address_state(Cellable* p): data_base(hsh::bc_address_state, p, 0, 0) {
        nonce = 0; balance = 0;
    }
    int64_t balance = 0;
    int64_t nonce   = 0;
    std::map<uint64_t, Key> keys;      // id -> Key
    std::set<CONTRACT_id> contracts;   // контракты, где адрес owner
    std::set<NODE_id>     owned_nodes; // ноды, где адрес owner
    std::set<NODE_id>     staked_in;   // ноды, где адрес стейкер

    size_t size() const {
        size_t sz = size_() + sizeof(balance) + sizeof(nonce);
        for (auto& [id, k] : keys) sz += sizeof(id) + sizeof(k.type) + k.keybuffer.size();
        for (auto& c : contracts)   sz += c.container.size();
        for (auto& n : owned_nodes) sz += n.container.size();
        for (auto& n : staked_in)   sz += n.container.size();
        return sz;
    }
    int64_t getNonce() const { return nonce; }
    void incNonce() { nonce += 1; }

    std::optional<Key> getKey(uint64_t id) const {
        auto it = keys.find(id);
        if (it == keys.end()) return std::nullopt;
        return it->second;
    }
    bool hasKeyId(uint64_t id) const { return keys.count(id) > 0; }

    std::optional<uint64_t> addKey(uint8_t type, const std::string& keybuffer) {
        if (keys.size() >= MAX_KEYS_PER_ADDRESS) return std::nullopt;
        uint64_t id = keys.empty() ? 0 : keys.rbegin()->first + 1;
        Key k; k.id = id; k.type = type; k.keybuffer = keybuffer;
        keys[id] = k;
        return id;
    }
    void removeKey(uint64_t id) { keys.erase(id); }
    size_t sizeKeys() const { return keys.size(); }

    void pack(outBuffer& o) const final {
        data_base::pack(o);
        o << 1 << nonce << balance << keys << contracts << owned_nodes << staked_in;
    }
    void unpack(inBuffer& o) final {
        data_base::unpack(o);
        auto v = o.get_PN();
        o >> nonce >> balance >> keys >> contracts >> owned_nodes >> staked_in;
    }
};