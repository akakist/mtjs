#pragma once
#include "md_Base.h"
namespace MsgData
{
    struct GetStateREQ: public Base
    {
        GetStateREQ():Base(msgid::GetStateREQ) {}
        void update(Blake2bHasher& h) const {}
        void pack(outBuffer& b) const final { Base::pack(b); }
        void unpack(inBuffer& b) final { Base::unpack(b); }
        static Base* construct() { return new GetStateREQ(); }
    };
}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::GetStateREQ> &s)
{ b<<1; s->pack(b); return b; }
inline inBuffer & operator>> (inBuffer& b, REF_getter<MsgData::GetStateREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid()) s=new MsgData::GetStateREQ();
    s->unpack2(b);
    return b;
}
