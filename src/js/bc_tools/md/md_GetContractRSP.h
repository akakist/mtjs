#pragma once
#include "md_Base.h"
#include "CONTRACT_id.h"
#include "ADDRESS_id.h"
#include <set>
namespace MsgData
{
    struct GetContractRSP: public Base
    {
        GetContractRSP():Base(msgid::GetContractRSP) {}
        bool found = false;
        CONTRACT_id contract;
        std::set<ADDRESS_id> owners;
        std::string src;
        int64_t ttl = -1;            // ← новое
        int64_t update_time = 0;     // ← новое
        void update(Blake2bHasher& h) const
        {
            h.update(found ? "1" : "0");
            h.update(contract.container);
            for (auto& o : owners) h.update(o.addr);
            h.update(src);
            h.update(std::to_string(ttl));
            h.update(std::to_string(update_time));
        }
        void pack(outBuffer& b) const final
        {
            Base::pack(b);
            b<<found<<contract<<owners<<src<<ttl<<update_time;
        }
        void unpack(inBuffer& b) final
        {
            Base::unpack(b);
            b>>found>>contract>>owners>>src>>ttl>>update_time;
        }
        static Base* construct() { return new GetContractRSP(); }
    };
}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::GetContractRSP> &s)
{
    b<<1; s->pack(b); return b;
}
inline inBuffer & operator>> (inBuffer& b, REF_getter<MsgData::GetContractRSP> &s)
{
    auto ver=b.get_PN();
    if(!s.valid()) s=new MsgData::GetContractRSP();
    s->unpack2(b);
    return b;
}