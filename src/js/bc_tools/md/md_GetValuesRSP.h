#pragma once
#include "md_Base.h"
#include <map>
#include "rsp_status.h"
namespace MsgData
{
    struct GetValuesRSP: public Base, public rsp_status
    {
        GetValuesRSP():Base(msgid::GetValuesRSP) {}
        std::map<std::string, int64_t> fees;
        void update(Blake2bHasher& h) const
        {
            update_status(h);
            for(auto& z: fees) { h.update(z.first); h.update(std::to_string(z.second)); }
        }
        void pack(outBuffer& b) const final
        {
            Base::pack(b);
            pack_status(b);
            b<<fees;
        }
        void unpack(inBuffer& b) final
        {
            Base::unpack(b);
            unpack_status(b);
            b>>fees;
        }
        static Base* construct() { return new GetValuesRSP(); }
    };
}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::GetValuesRSP> &s)
{
    b<<1; s->pack(b); return b;
}
inline inBuffer & operator>> (inBuffer& b, REF_getter<MsgData::GetValuesRSP> &s)
{
    auto ver=b.get_PN();
    if(!s.valid()) s=new MsgData::GetValuesRSP();
    s->unpack2(b);
    return b;
}
