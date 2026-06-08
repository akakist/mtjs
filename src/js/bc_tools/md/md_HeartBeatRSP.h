#pragma once
#include "md_Base.h"
#include "md_HeartBeatREQ.h"
#include "blst_cp.h"
namespace MsgData
{
    struct HeartBeatRSP: public Base
    {

        HeartBeatRSP():Base(msgid::HeartBeatRSP), payload_heart_beat(new HeartBeatREQ())
        {
        }
        REF_getter<HeartBeatREQ> payload_heart_beat;
        NODE_id node_signer;
        blst_cpp::Signature signature;
        void update(Blake2bHasher& h) const
        {
            payload_heart_beat->update(h);
            h.update(node_signer.container);
        }
        static Base* construct()
        {
            return new HeartBeatRSP();
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            b<<payload_heart_beat;
            b<<node_signer;
            b<<signature;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>payload_heart_beat;
            b>>node_signer;
            b>>signature;
        }
    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::HeartBeatRSP> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::HeartBeatRSP> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::HeartBeatRSP();
    s->unpack2(b);
    return b;
}
