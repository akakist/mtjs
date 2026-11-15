#pragma once
#include "md_Base.h"
#include "rsp_status.h"

namespace MsgData
{
    struct GetUserNonceRSP: public Base, public rsp_status
    {

        GetUserNonceRSP():Base(msgid::GetUserNonceRSP)
        {
        }
        int64_t balance;
        int64_t nonce;
        void update(Blake2bHasher& h) const
        {
            update_status(h);
            h.update(std::to_string(balance));
            h.update(std::to_string(nonce));
        }

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            pack_status(b);
            b
            << balance
            << nonce;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            unpack_status(b);
            b
            >> balance
            >> nonce;
        }

        static Base* construct()
        {
            return new GetUserNonceRSP();
        }
    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::GetUserNonceRSP> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::GetUserNonceRSP> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::GetUserNonceRSP();
    s->unpack2(b);
    return b;
}
