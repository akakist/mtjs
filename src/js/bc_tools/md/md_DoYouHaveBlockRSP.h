#pragma once
#include "md_Base.h"
#include "BLOCK_id.h"
#include "bigint.h"
#include "NODE_id.h"
#include <nlohmann/json.hpp>
namespace MsgData
{
    struct DoYouHaveBlockRSP: public Base
    {

        DoYouHaveBlockRSP():Base(msgid::DoYouHaveBlockRSP)
        {

        }
        DoYouHaveBlockRSP(bool _found, const BLOCK_id& _prev_root_hash ):Base(msgid::DoYouHaveBlockRSP),
            found(_found), prev_root_hash(_prev_root_hash)
        {
        }
        bool found;
        BLOCK_id prev_root_hash;
        void dump(nlohmann::json& j)
        {
            j["found"]=found;
            j["prev_root_hash"]=prev_root_hash.container;
            
        }

        void update(Blake2bHasher& h) const
        {
            h.update(std::to_string(found));
            h.update(prev_root_hash.container);
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            b<<found;
            b<<prev_root_hash;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>found;
            b>>prev_root_hash;
        }
        static Base* construct()
        {
            return new DoYouHaveBlockRSP();
        }

    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::DoYouHaveBlockRSP> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::DoYouHaveBlockRSP> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::DoYouHaveBlockRSP();
    s->unpack2(b);
    return b;
}
