#pragma once
#include "md_Base.h"
#include "CONTRACT_id.h"
#include "ADDRESS_id.h"
#include <set>
#include "rsp_status.h"

namespace MsgData
{
    struct GetContractRSP: public Base, public rsp_status
    {
        GetContractRSP():Base(msgid::GetContractRSP) { setError(msgcode::NOT_FOUND, "not found"); }
        CONTRACT_id contract;
        std::set<ADDRESS_id> owners;
        std::string src;
        int64_t ttl = -1;            // ← новое
        int64_t update_time = 0;     // ← новое
        void update(Blake2bHasher& h) const
        {
            update_status(h);
            h.update(contract.container);
            for (auto& o : owners) h.update(o.addr);
            h.update(src);
            h.update(std::to_string(ttl));
            h.update(std::to_string(update_time));
        }
        void pack(outBuffer& b) const final
        {
            Base::pack(b);
            pack_status(b);
            b<<contract<<owners<<src<<ttl<<update_time;
        }
        void unpack(inBuffer& b) final
        {
            Base::unpack(b);
            unpack_status(b);
            b>>contract>>owners>>src>>ttl>>update_time;
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