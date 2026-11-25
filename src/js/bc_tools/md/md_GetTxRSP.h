#pragma once
#include "md_Base.h"
#include "md_TX.h"
#include "rsp_status.h"

namespace MsgData
{
    struct GetTxRSP: public Base
    {

        static Base* construct()
        {
            return new GetTxRSP();
        }
        GetTxRSP():Base(msgid::GetTxRSP)
        {
        }

        std::vector<REF_getter<MsgData::TX>>  trs;
        void update(Blake2bHasher& h) const
        {
            for(auto& z:trs )
            {
                h.update(z->tx_body);

            }
        }

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            b<<trs;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>trs;
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

