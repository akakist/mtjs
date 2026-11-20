#pragma once
#include "md_Base.h"
#include "rsp_status.h"
namespace MsgData
{
    struct GetContractDataRSP: public Base, public rsp_status
    {
        GetContractDataRSP():Base(msgid::GetContractDataRSP) { setError(msgcode::NOT_FOUND, "not found"); }
        std::string contract;
        std::string key;
        std::string value;
        void update(Blake2bHasher& h) const
        {
            update_status(h);
            h.update(contract); h.update(key); h.update(value);
        }
        void pack(outBuffer& b) const final
        {
            Base::pack(b);
            pack_status(b);
            b<<contract<<key<<value;
        }
        void unpack(inBuffer& b) final
        {
            Base::unpack(b);
            unpack_status(b);
            b>>contract>>key>>value;
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
