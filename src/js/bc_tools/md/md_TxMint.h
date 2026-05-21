#pragma once
#include "md_Base.h"
namespace MsgData
{
    struct TxMint: public Base
    {
        TxMint():Base(msgid::TxMint) {}
        BigInt amount;
        static Base* construct()
        {
            return new TxMint();
        }

        void update(Blake2bHasher &h) const
        {
            h.update(amount.toString());
        }

        void pack(outBuffer& b) const final
        {
            Base::pack(b);
            b<<amount;
        }
        void unpack(inBuffer& b) final
        {
            Base::unpack(b);
            b>>amount;
        }
    };


}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::TxMint> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData  ::TxMint> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::TxMint();
    s->unpack2(b);
    return b;
}   
