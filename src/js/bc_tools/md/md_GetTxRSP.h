#pragma once
#include "md_Base.h"
#include "md_TX.h"
#include "rsp_status.h"

namespace MsgData
{
    struct GetTxRSP: public Base, public rsp_status
    {

        static Base* construct()
        {
            return new GetTxRSP();
        }
        GetTxRSP():Base(msgid::GetTxRSP)
        {

        }
        GetTxRSP(inBuffer &in):Base(msgid::GetTxRSP)
        {
            unpack(in);
        }

        std::vector<REF_getter<MsgData::TX>>  trs;
        REF_getter<MsgData::HeartBeatREQ> hb;
        void update(Blake2bHasher& h) const
        {
            for(auto& z:trs )
                h.update(z->tx_body);
            hb->update(h);
        }

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            b<<trs;
            b<< hb;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>trs;
            b>> hb;
        }

    };


}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::GetTxRSP> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::GetTxRSP> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::GetTxRSP();
    s->unpack2(b);
    return b;
}

