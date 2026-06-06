#pragma once
#include "md_Base.h"
namespace MsgData
{
    struct HeartBeatREQ: public Base
    {

        HeartBeatREQ():Base(msgid::HeartBeatREQ)
        {

        }
        HeartBeatREQ(const BLOCK_id& _prev_block_hash, const BigInt& _newepoch, const NODE_id& _node_leader):Base(msgid::HeartBeatREQ),
            prev_root_hash(_prev_block_hash), new_epoch(_newepoch), node_leader(_node_leader)
        {
        }
        BLOCK_id prev_root_hash;
        BigInt new_epoch;
        NODE_id node_leader;
        void update(Blake2bHasher& h) const
        {
            h.update(prev_root_hash.container);
            h.update(new_epoch.toString());
            h.update(node_leader.container);
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            b<<prev_root_hash;
            b<<node_leader;
            b<<new_epoch;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>prev_root_hash;
            b>>node_leader;
            b>>new_epoch;
        }
        static Base* construct()
        {
            return new HeartBeatREQ();
        }

    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::HeartBeatREQ> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::HeartBeatREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::HeartBeatREQ();
    s->unpack2(b);
    return b;
}
