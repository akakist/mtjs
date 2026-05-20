#pragma once
#include "md_Base.h"
#include "md_HeartBeatREQ.h"
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
            payload_heart_beat->hash(h);
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
            payload_heart_beat->pack(b);
            b<<node_signer;
            b<<signature;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            payload_heart_beat->unpack2(b);
            b>>node_signer;
            b>>signature;
        }
    };

}
