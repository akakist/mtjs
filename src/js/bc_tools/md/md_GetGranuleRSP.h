#pragma once
#include "md_Base.h"
#include "rsp_status.h"

namespace MsgData
{
    struct GetGranuleRSP: public Base, public rsp_status
    {
        GetGranuleRSP():Base(msgid::GetGranuleRSP) {}
        bool stale = false;     // true — из stale_cache (этап 2)
        std::string body;       // тело гранулы (бинарный)
        void update(Blake2bHasher& h) const
        {
            update_status(h);
            h.update(stale ? "1" : "0");
            h.update(body);
        }
        void pack(outBuffer& b) const final
        {
            Base::pack(b);
            pack_status(b);
            b<<stale<<body;
        }
        void unpack(inBuffer& b) final
        {
            Base::unpack(b);
            unpack_status(b);
            b>>stale>>body;
        }
        static Base* construct() { return new GetGranuleRSP(); }
    };
}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::GetGranuleRSP> &s)
{
    b<<1; s->pack(b); return b;
}
inline inBuffer & operator>> (inBuffer& b, REF_getter<MsgData::GetGranuleRSP> &s)
{
    auto ver=b.get_PN();
    if(!s.valid()) s=new MsgData::GetGranuleRSP();
    s->unpack2(b);
    return b;
}
