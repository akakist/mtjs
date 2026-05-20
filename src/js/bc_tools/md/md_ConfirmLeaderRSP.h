#pragma once
#include "md_Base.h"
#include "md_HeartBeatREQ.h"
#include "blst_cp.h"
namespace MsgData
{
    struct ConfirmLeaderRSP: public Base
    {
        
        static Base* construct()
        {
            return new ConfirmLeaderRSP();
        }
        ConfirmLeaderRSP():Base(msgid::ConfirmLeaderRSP), hb(new HeartBeatREQ)
        {

        }
        REF_getter<HeartBeatREQ> hb;
        blst_cpp::Signature sig;        
        NODE_id node_signer;
        void update(Blake2bHasher& h) const
        {
            throw CommonError("unimp");
        }

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            hb->pack(b);
            b<<sig<<node_signer;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            hb->unpack2(b);
            b>>sig>>node_signer;
        }
    };

}
