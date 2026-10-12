#pragma once
#include "md_Base.h"

#include "md_BlockInfo.h"
#include "blst_cp.h"
namespace MsgData
{
    struct LeaderCert: public Base
    {

        LeaderCert()
            : Base(msgid::LeaderCert),
            hb(new HeartBeatREQ)
        {
        }

        static Base* construct()
        {
            return new LeaderCert();
        }
        REF_getter<HeartBeatREQ> hb;
        std::set<NODE_id> node_validators;
        blst_cpp::AggregateSignature agg_sig;
        void update(Blake2bHasher& h) const;
        void pack(outBuffer& b) const final;
        void unpack(inBuffer& b) final;

    };

}
inline void MsgData::LeaderCert::pack(outBuffer &b) const
{
    XTRY;
    MUTEX_INSPECTOR;
    Base::pack(b);
    b << hb;
    b << node_validators << agg_sig;
    XPASS;
}
inline void MsgData::LeaderCert::unpack(inBuffer &b)
{
    XTRY;
    MUTEX_INSPECTOR;
    Base::unpack(b);
    b >> hb;
    b >> node_validators >> agg_sig;
    XPASS;
}

inline void MsgData::LeaderCert::update(Blake2bHasher &h) const
{
    hb->update(h);
    for (auto &z : node_validators)
    {
        h.update(z.container);
    }
}

inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::LeaderCert> &s)
{
    MUTEX_INSPECTOR;
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::LeaderCert> &s)
{
    MUTEX_INSPECTOR;
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::LeaderCert();
    s->unpack2(b);
    return b;
}