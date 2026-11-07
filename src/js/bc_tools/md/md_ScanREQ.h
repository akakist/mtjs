#pragma once
#include "md_Base.h"
#include "THASH_id.h"
namespace MsgData
{
    struct ScanREQ: public Base
    {
        ScanREQ():Base(msgid::ScanREQ) {}
        THASH_id from;       // dbid, с которого начать (пусто = с начала)
        int64_t max_sec = 5; // лимит времени сканирования
        int64_t now = 0;     // 0 = time(NULL)
        void update(Blake2bHasher& h) const { h.update(from.container); h.update(std::to_string(max_sec)); h.update(std::to_string(now)); }
        void pack(outBuffer& b) const final { Base::pack(b); b<<from<<max_sec<<now; }
        void unpack(inBuffer& b) final { Base::unpack(b); b>>from>>max_sec>>now; }
        static Base* construct() { return new ScanREQ(); }
    };
}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::ScanREQ> &s) { b<<1; s->pack(b); return b; }
inline inBuffer & operator>> (inBuffer& b, REF_getter<MsgData::ScanREQ> &s) { auto ver=b.get_PN(); if(!s.valid()) s=new MsgData::ScanREQ(); s->unpack2(b); return b; }
