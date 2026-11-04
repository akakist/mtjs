#pragma once
#include "md_Base.h"
#include "ADDRESS_id.h"
#include <vector>
namespace MsgData {
struct KeyInfo {
    uint64_t    id = 0;
    uint8_t     type = 0;
    std::string keybuffer;
    void pack(outBuffer& o) const { o << id << type << keybuffer; }
    void unpack(inBuffer& in)  { in >> id >> type >> keybuffer; }
};
inline outBuffer& operator<<(outBuffer& o, const KeyInfo& k) { k.pack(o); return o; }
inline inBuffer&  operator>>(inBuffer& in, KeyInfo& k) { k.unpack(in); return in; }

struct GetAddressInfoRSP: public Base {
    GetAddressInfoRSP():Base(msgid::GetAddressInfoRSP) {}
    bool found = false;
    int64_t balance = 0;
    int64_t nonce = 0;
    std::vector<KeyInfo> keys;
    void update(Blake2bHasher& h) const {
        h.update(found ? "1" : "0");
        h.update(std::to_string(balance));
        h.update(std::to_string(nonce));
        for (auto& k : keys) {
            h.update(std::to_string(k.id));
            h.update(std::to_string(k.type));
            h.update(k.keybuffer);
        }
    }
    void pack(outBuffer& b) const final { Base::pack(b); b << found <<  balance << nonce << keys; }
    void unpack(inBuffer& b) final { Base::unpack(b); b >> found >>  balance >> nonce >> keys; }
    static Base* construct() { return new GetAddressInfoRSP(); }
};
}
inline outBuffer& operator<<(outBuffer& b, const REF_getter<MsgData::GetAddressInfoRSP>& s) { b<<1; s->pack(b); return b; }
inline inBuffer&  operator>>(inBuffer& b, REF_getter<MsgData::GetAddressInfoRSP>& s) {
    auto v = b.get_PN(); if(!s.valid()) s = new MsgData::GetAddressInfoRSP(); s->unpack2(b); return b;
}
