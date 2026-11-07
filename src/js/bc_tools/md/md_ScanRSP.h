#pragma once
#include "md_Base.h"
#include "THASH_id.h"
namespace MsgData
{
    struct ScanRSP: public Base
    {
        ScanRSP():Base(msgid::ScanRSP) {}
        std::vector<THASH_id> dbids;
        THASH_id next_from;
        bool done = false;
        int64_t scanned = 0;
        void update(Blake2bHasher& h) const {
            for (auto& d : dbids) h.update(d.container);
            h.update(next_from.container);
            h.update(done ? "1" : "0");
            h.update(std::to_string(scanned));
        }
        void pack(outBuffer& b) const final { Base::pack(b); b<<dbids<<next_from<<done<<scanned; }
        void unpack(inBuffer& b) final { Base::unpack(b); b>>dbids>>next_from>>done>>scanned; }
        static Base* construct() { return new ScanRSP(); }
    };
}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::ScanRSP> &s) { b<<1; s->pack(b); return b; }
inline inBuffer & operator>> (inBuffer& b, REF_getter<MsgData::ScanRSP> &s) { auto ver=b.get_PN(); if(!s.valid()) s=new MsgData::ScanRSP(); s->unpack2(b); return b; }
