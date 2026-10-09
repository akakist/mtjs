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
        BlockInfo(const THASH_id& _new_root_hash,
        const THASH_id & _tx_hash,
        const THASH_id & _diffs_hash,
        const THASH_id & _att_hash,
        const REF_getter<HeartBeatREQ> &_heart_beat
        // ,
        // const std::map<NODE_id, std::map<char, uint64_t>>& _node_stake_changes,
        // const std::map<NODE_id, int> &_node_enables
        ):Base(msgid::BlockInfo),
        new_root_hash1(_new_root_hash),
        tx_hash(_tx_hash),
        diffs_hash(_diffs_hash),
        att_data_hash(_att_hash),
        heart_beat(_heart_beat)
        // ,
        // node_stake_changes(_node_stake_changes),
        // node_enables(_node_enables)
        {

        }
        THASH_id new_root_hash1;
        THASH_id tx_hash;
        THASH_id diffs_hash;
        THASH_id att_data_hash;
        REF_getter<HeartBeatREQ> heart_beat;
        // std::map<NODE_id, std::map<char, uint64_t>> node_stake_changes;
        // std::map<NODE_id, int> node_enables;

        void update(Blake2bHasher& h) const
        {
            MUTEX_INSPECTOR;
            h.update(new_root_hash1.container);
            h.update(tx_hash.container);
            h.update(diffs_hash.container);
            h.update(att_data_hash.container);
           
            heart_beat->update(h);
            // for(auto &z: node_stake_changes)
            // {
            //     h.update(z.first.container);
            //     for(auto &y: z.second)
            //     {
            //         h.update(std::to_string(y.first));
            //         h.update(std::to_string(y.second));
            //     }
            // }
            // for(auto &z: node_enables)
            // {
            //     h.update(z.first.container);
            //     h.update(std::to_string(z.second));
            // }
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
            // b<<node_stake_changes;
            // b<<node_enables;
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
            // b>>node_stake_changes;
            // b>>node_enables;
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
