#pragma once
#include "md_Base.h"
namespace MsgData
{
    struct GetValuesREQ: public Base
    {
        GetValuesREQ():Base(msgid::GetValuesREQ) {}
        void update(Blake2bHasher& h) const {}
        void pack(outBuffer& b) const final { Base::pack(b); }
        void unpack(inBuffer& b) final { Base::unpack(b); }
        static Base* construct() { return new GetValuesREQ(); }
    };
}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::GetValuesREQ> &s)
{
    b<<1; s->pack(b); return b;
}
inline inBuffer & operator>> (inBuffer& b, REF_getter<MsgData::GetValuesREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid()) s=new MsgData::GetValuesREQ();
    s->unpack2(b);
    return b;
}
