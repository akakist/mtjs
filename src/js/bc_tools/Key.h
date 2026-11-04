#pragma once
#include <cstdint>
#include <string>
#include "ioBuffer.h"

enum KeyType : uint8_t {
    KEY_ED25519   = 0,
    KEY_SECP256K1 = 1,
    KEY_P256      = 2,
};

struct Key {
    uint64_t    id   = 0;
    uint8_t     type = KEY_ED25519;
    std::string keybuffer;

    void pack(outBuffer& o) const { o << id << type << keybuffer; }
    void unpack(inBuffer& in)  { in >> id >> type >> keybuffer; }

    bool operator==(const Key& o) const {
        return id == o.id && type == o.type && keybuffer == o.keybuffer;
    }
};
inline outBuffer& operator<<(outBuffer& o, const Key& k) { k.pack(o); return o; }
inline inBuffer&  operator>>(inBuffer& in, Key& k) { k.unpack(in); return in; }
