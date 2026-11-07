#pragma once
#include "md_Base.h"
namespace MsgData
{
    struct GetGranuleREQ: public Base
    {
        GetGranuleREQ():Base(msgid::GetGranuleREQ) {}
        std::string addr;   // dbId, 32 байта (бинарный)
        std::string hash;   // blake2b(body), 32 байта (бинарный); пусто — не проверять
        void update(Blake2bHasher& h) const
        {
            h.update(addr);
            h.update(hash);
        }
        void pack(outBuffer& b) const final
        {
            Base::pack(b);
            b<<addr<<hash;
        }
        void unpack(inBuffer& b) final
        {
            Base::unpack(b);
            b>>addr>>hash;
        }
        static Base* construct() { return new GetGranuleREQ(); }
    };
}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::GetGranuleREQ> &s)
{
    b<<1; s->pack(b); return b;
}
inline inBuffer & operator>> (inBuffer& b, REF_getter<MsgData::GetGranuleREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid()) s=new MsgData::GetGranuleREQ();
    s->unpack2(b);
    return b;
}