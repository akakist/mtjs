#pragma once
#include "md_Base.h"
#include "BLOCK_id.h"
#include "bigint.h"
#include "NODE_id.h"
#include "EPOCH_id.h"
#include <nlohmann/json.hpp>
namespace MsgData
{
    struct HeartBeatREQ: public Base
    {

        HeartBeatREQ():Base(msgid::HeartBeatREQ)
        {

        }
        HeartBeatREQ(const BLOCK_id& _prev_block_hash, const EPOCH_id& _newepoch, const NODE_id& _node_leader, const std::string& _prev_lc, time_t _block_ts):Base(msgid::HeartBeatREQ),
            prev_root_hash_1(_prev_block_hash), new_epoch(_newepoch), node_leader(_node_leader), block_timestamp(_block_ts)
        {
        }
        BLOCK_id prev_root_hash_1;
        EPOCH_id new_epoch;
        NODE_id node_leader;
        time_t block_timestamp;
        void dump(nlohmann::json& j)
        {
            j["prev_root_hash"]=prev_root_hash_1.str();
            j["new_epoch"]=new_epoch.container;
            j["block_timestamp"]=block_timestamp;
            j["node_leader"]=node_leader.container;
            
        }

        void update(Blake2bHasher& h) const
        {
            h.update(prev_root_hash_1.container);
            h.update(std::to_string(new_epoch.container));
            h.update(node_leader.container);
            h.update(std::to_string(block_timestamp));
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            b<<prev_root_hash_1;
            b<<node_leader;
            b<<new_epoch;
            b<<block_timestamp;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>prev_root_hash_1;
            b>>node_leader;
            b>>new_epoch;
            b>>block_timestamp;
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
