#pragma once
#include "md_Base.h"
#include "ADDRESS_id.h"
namespace MsgData {
struct GetAddressInfoREQ: public Base {
    GetAddressInfoREQ():Base(msgid::GetAddressInfoREQ) {}
    ADDRESS_id addr;
    void update(Blake2bHasher& h) const { h.update(addr.addr); }
    void pack(outBuffer& b) const final { Base::pack(b); b << addr; }
    void unpack(inBuffer& b) final { Base::unpack(b); b >> addr; }
    static Base* construct() { return new GetAddressInfoREQ(); }
};
}
inline outBuffer& operator<<(outBuffer& b, const REF_getter<MsgData::GetAddressInfoREQ>& s) { b<<1; s->pack(b); return b; }
inline inBuffer&  operator>>(inBuffer& b, REF_getter<MsgData::GetAddressInfoREQ>& s) {
    auto v = b.get_PN(); if(!s.valid()) s = new MsgData::GetAddressInfoREQ(); s->unpack2(b); return b;
}
