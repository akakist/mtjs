#pragma once
#include "md_Base.h"
#include "md_LeaderCertificate.h"
namespace MsgData
{
    struct getAddressStateRSP: public Base
    {

        getAddressStateRSP():Base(msgid::getAddressStateRSP)
        {
        }
        // BigInt balance;
        uint64_t nonce;
        void update(Blake2bHasher& h) const
        {
            // h.update(balance.toString());
            h.update(std::to_string(nonce));
        }

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            b
            // <<balance
            <<nonce;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b
            // >>balance
            >>nonce;
        }

        static Base* construct()
        {
            return new getAddressStateRSP();
        }
    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::getAddressStateRSP> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::getAddressStateRSP> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::getAddressStateRSP();
    s->unpack2(b);
    return b;
}
