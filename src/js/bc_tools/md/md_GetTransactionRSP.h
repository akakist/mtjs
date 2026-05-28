#pragma once
#include "md_Base.h"
#include "md_TX.h"
namespace MsgData
{
    struct GetTransactionRSP: public Base
    {

        static Base* construct()
        {
            return new GetTransactionRSP();
        }
        GetTransactionRSP():Base(msgid::GetTransactionRSP)
        {

        }
        GetTransactionRSP(inBuffer &in):Base(msgid::GetTransactionRSP)
        {
            unpack(in);
        }

        std::vector<REF_getter<MsgData::TX>>  trs;
        void update(Blake2bHasher& h) const
        {
            for(auto& z:trs )
                h.update(z->hash.container);
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
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::GetTransactionRSP> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::GetTransactionRSP> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::GetTransactionRSP();
    s->unpack2(b);
    return b;
}

