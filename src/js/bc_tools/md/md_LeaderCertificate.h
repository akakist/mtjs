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
            b<<heart_beat;
            b<<nodes;
            b<<agg_sig;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>heart_beat;
            b>>nodes;
            b>>agg_sig;
        }
        static Base* construct()
        {
            return new LeaderCertificate();
        }

    };

}

inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::LeaderCertificate> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::LeaderCertificate> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::LeaderCertificate();
    s->unpack2(b);
    return b;
}
