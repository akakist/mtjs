#pragma once
#include "md_Base.h"
#include "md_HeartBeatREQ.h"
namespace MsgData
{
    struct ConfirmLeaderREQ: public Base
    {
        
        static Base* construct()
        {
            return new ConfirmLeaderREQ();
        }
        ConfirmLeaderREQ():Base(msgid::ConfirmLeaderREQ), hb(new HeartBeatREQ)
        {

        }
        REF_getter<HeartBeatREQ> hb;
        void update(Blake2bHasher& h) const
        {
            throw CommonError("unimp");
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            hb->pack(b);
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            hb->unpack2(b);
        }
    };

}
