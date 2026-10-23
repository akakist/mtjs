#pragma once
#include "md_Base.h"
#include "NODE_id.h"
namespace MsgData
{
    struct GetNodeInfoREQ: public Base
    {
        GetNodeInfoREQ():Base(msgid::GetNodeInfoREQ) {}
        NODE_id node_name;
        void update(Blake2bHasher& h) const
        {
            h.update(node_name.container);
        }
        void pack(outBuffer& b) const final
        {
            Base::pack(b);
            b<<node_name;
        }
        void unpack(inBuffer& b) final
        {
            Base::unpack(b);
            b>>node_name;
        }
        static Base* construct() { return new GetNodeInfoREQ(); }
    };
}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::GetNodeInfoREQ> &s)
{
    b<<1; s->pack(b); return b;
}
inline inBuffer & operator>> (inBuffer& b, REF_getter<MsgData::GetNodeInfoREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid()) s=new MsgData::GetNodeInfoREQ();
    s->unpack2(b);
    return b;
}
