#pragma once
#include "md_Base.h"
namespace MsgData
{
    struct GetContractDataRSP: public Base
    {
        GetContractDataRSP():Base(msgid::GetContractDataRSP) {}
        bool found = false;
        std::string contract;
        std::string key;
        std::string value;
        void update(Blake2bHasher& h) const
        {
            h.update(found ? "1" : "0");
            h.update(contract); h.update(key); h.update(value);
        }
        void pack(outBuffer& b) const final
        {
            Base::pack(b);
            b<<found<<contract<<key<<value;
        }
        void unpack(inBuffer& b) final
        {
            Base::unpack(b);
            b>>found>>contract>>key>>value;
        }
        static Base* construct() { return new GetContractDataRSP(); }
    };
}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::GetContractDataRSP> &s)
{
    b<<1; s->pack(b); return b;
}
inline inBuffer & operator>> (inBuffer& b, REF_getter<MsgData::GetContractDataRSP> &s)
{
    auto ver=b.get_PN();
    if(!s.valid()) s=new MsgData::GetContractDataRSP();
    s->unpack2(b);
    return b;
}
