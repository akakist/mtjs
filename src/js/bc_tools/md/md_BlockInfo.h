#pragma once
#include "md_Base.h"
#include "md_HeartBeatREQ.h"
#include "md_attachment_data.h"
#include <nlohmann/json.hpp>
namespace MsgData
{
    struct BlockInfo: public Base
    {

        static Base* construct()
        {
            return new BlockInfo();
        }
        BlockInfo():Base(msgid::BlockInfo),heart_beat(new HeartBeatREQ())
        {

        }
        THASH_id new_root_hash1;
        THASH_id tx_hash;
        THASH_id diffs_hash;
        THASH_id att_data_hash;
        REF_getter<HeartBeatREQ> heart_beat;
        void update(Blake2bHasher& h) const
        {
            MUTEX_INSPECTOR;
            h.update(new_root_hash1.container);
            h.update(tx_hash.container);
            h.update(diffs_hash.container);
            h.update(att_data_hash.container);
           
            heart_beat->update(h);
        }

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            b<<new_root_hash1;
            b<<att_data_hash;
            b<<tx_hash;
            b<<diffs_hash;
            b<<heart_beat;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>new_root_hash1;
            b>>att_data_hash;
            b>>tx_hash;
            b>>diffs_hash;
            b>>heart_beat;
        }

    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::BlockInfo> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::BlockInfo> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::BlockInfo();
    s->unpack2(b);
    return b;
}
