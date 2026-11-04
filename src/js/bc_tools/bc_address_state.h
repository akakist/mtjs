#pragma once
#include "cellable.h"
#include "Key.h"
#include <map>
#include <optional>

static constexpr size_t MAX_KEYS_PER_ADDRESS = 5;

struct bc_address_state: public data_base {
    bc_address_state(Cellable* p): data_base(hsh::bc_address_state, p, 0, -1) {
        nonce = 0; balance = 0;
    }
    int64_t balance = 0;
    int64_t nonce   = 0;
    std::map<uint64_t, Key> keys;   // id -> Key

    size_t size() const {
        size_t sz = size_() + sizeof(balance) + sizeof(nonce);
        for (auto& [id, k] : keys) sz += sizeof(id) + sizeof(k.type) + k.keybuffer.size();
        return sz;
    }
    int64_t getNonce() const { M_LOCK(parent->mx); return nonce; }
    void incNonce()    { M_LOCK(parent->mx); nonce += 1; }

    std::optional<Key> getKey(uint64_t id) const {
        M_LOCK(parent->mx);
        auto it = keys.find(id);
        if (it == keys.end()) return std::nullopt;
        return it->second;
    }
    bool hasKeyId(uint64_t id) const {
        M_LOCK(parent->mx);
        return keys.count(id) > 0;
    }
    // id авто: max + 1
    std::optional<uint64_t> addKey(uint8_t type, const std::string& keybuffer) {
        M_LOCK(parent->mx);
        if (keys.size() >= MAX_KEYS_PER_ADDRESS) return std::nullopt;
        uint64_t id = keys.empty() ? 0 : keys.rbegin()->first + 1;
        Key k; k.id = id; k.type = type; k.keybuffer = keybuffer;
        keys[id] = k;
        return id;
    }
    void removeKey(uint64_t id) {
        M_LOCK(parent->mx);
        keys.erase(id);
    }
    size_t sizeKeys() const { M_LOCK(parent->mx); return keys.size(); }

    void pack(outBuffer& o) const final {
        data_base::pack(o);
        o << 1 << nonce << balance << keys;
    }
    void unpack(inBuffer& o) final {
        data_base::unpack(o);
        auto v = o.get_PN();
        o >> nonce >> balance >> keys;
    }
};
