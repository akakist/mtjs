#pragma once
#include "md_Base.h"
#include "md_HeartBeatREQ.h"
namespace MsgData
{
    struct BlockInfo: public Base
    {
        
        static Base* construct()
        {
            return new BlockInfo();
        }
        BlockInfo():Base(msgid::BlockInfo),payload_heart_beat(new HeartBeatREQ())
        {

        }
        BigInt prev_epoch;
        BLOCK_id prev_root_hash;
        BLOCK_id new_root_hash1;
        THASH_id attachment_hash;
        REF_getter<HeartBeatREQ> payload_heart_beat;
        void update(Blake2bHasher& h) const
        {
            h.update(prev_epoch.toString());
            h.update(prev_root_hash.container);
            h.update(new_root_hash1.container);
            h.update(attachment_hash.container);
            payload_heart_beat->update(h);
        }

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            b<<prev_epoch;
            b<<prev_root_hash;
            b<<new_root_hash1;
            b<<attachment_hash;
            b<<payload_heart_beat;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>prev_epoch;
            b>>prev_root_hash;
            b>>new_root_hash1;
            b>>attachment_hash;
            b>>payload_heart_beat;
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
