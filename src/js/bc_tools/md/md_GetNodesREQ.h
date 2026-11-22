#pragma once
#include "md_Base.h"
namespace MsgData
{
    struct GetNodesREQ: public Base
    {
        GetNodesREQ():Base(msgid::GetNodesREQ) {}
        void update(Blake2bHasher& h) const {}
        void pack(outBuffer& b) const final { Base::pack(b); }
        void unpack(inBuffer& b) final { Base::unpack(b); }
        static Base* construct() { return new GetNodesREQ(); }
    };
}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::GetNodesREQ> &s)
{
    b<<1; s->pack(b); return b;
}
inline inBuffer & operator>> (inBuffer& b, REF_getter<MsgData::GetNodesREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid()) s=new MsgData::GetNodesREQ();
    s->unpack2(b);
    return b;
}
