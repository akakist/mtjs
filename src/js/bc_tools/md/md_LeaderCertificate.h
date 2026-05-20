#pragma once
#include "md_Base.h"
#include "md_HeartBeatREQ.h"
namespace MsgData
{
    struct LeaderCertificate: public Base
    {
        
        LeaderCertificate():Base(msgid::LeaderCertificate), heart_beat(new MsgData::HeartBeatREQ())
        {

        }
        REF_getter<MsgData::HeartBeatREQ> heart_beat;
        std::vector<NODE_id> nodes;
        blst_cpp::AggregateSignature agg_sig;
        void update(Blake2bHasher& h) const
        {
            heart_beat->update(h);
            for(auto& z: nodes)
            {
                h.update(z.container);
            }
        }

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            heart_beat->pack(b);
            // b<<payload_heart_beat;
            b<<nodes;
            b<<agg_sig;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            // auto t=b.get_PN();
            // if(t!=msgid::HeartBeatREQ)                throw CommonError("if(t!=msgid::HeartBeatREQ)");
            heart_beat->unpack2(b);
            // b>>payload_heart_beat;
            b>>nodes;
            b>>agg_sig;
        }
        static Base* construct()
        {
            return new LeaderCertificate();
        }

    };

}
