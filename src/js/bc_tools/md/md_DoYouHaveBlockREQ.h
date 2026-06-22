#pragma once
#include "md_Base.h"
#include "BLOCK_id.h"
#include "bigint.h"
#include "NODE_id.h"
#include <nlohmann/json.hpp>
namespace MsgData
{
    struct DoYouHaveBlockREQ: public Base
    {

        DoYouHaveBlockREQ():Base(msgid::DoYouHaveBlockREQ)
        {

        }
        DoYouHaveBlockREQ(const BLOCK_id& _prev_block_hash):Base(msgid::DoYouHaveBlockREQ),
            prev_root_hash(_prev_block_hash)
        {
        }
        BLOCK_id prev_root_hash;
        void dump(nlohmann::json& j)
        {
            j["prev_root_hash"]=prev_root_hash.str();
            
        }

        void update(Blake2bHasher& h) const
        {
            h.update(prev_root_hash.container);
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            b<<prev_root_hash;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>prev_root_hash;
        }
        static Base* construct()
        {
            return new DoYouHaveBlockREQ();
        }

    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::DoYouHaveBlockREQ> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::DoYouHaveBlockREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::DoYouHaveBlockREQ();
    s->unpack2(b);
    return b;
}
