#pragma once
#include "md_Base.h"
#include "THASH_id.h"

#include "NODE_id.h"
#include <nlohmann/json.hpp>
namespace MsgData
{
    struct HelloRSP: public Base
    {

        HelloRSP():Base(msgid::HelloRSP)
        {

        }
        std::string prev_block;
        
        void update(Blake2bHasher& h) const
        {
            MUTEX_INSPECTOR;
            h.update(prev_block);
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            b<<prev_block;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>prev_block;
        }
        static Base* construct()
        {
            return new HelloRSP();
        }

    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::HelloRSP> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::HelloRSP> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::HelloRSP();
    s->unpack2(b);
    return b;
}
