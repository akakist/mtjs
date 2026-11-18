#pragma once
#include "md_Base.h"
#include "THASH_id.h"

#include "NODE_id.h"
#include <nlohmann/json.hpp>
namespace MsgData
{
    struct HelloREQ: public Base
    {

        HelloREQ():Base(msgid::HelloREQ)
        {

        }
        void update(Blake2bHasher& h) const
        {
            MUTEX_INSPECTOR;
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
        }
        static Base* construct()
        {
            return new HelloREQ();
        }

    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::HelloREQ> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::HelloREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::HelloREQ();
    s->unpack2(b);
    return b;
}
