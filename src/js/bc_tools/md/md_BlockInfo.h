#pragma once
#include "md_Base.h"
#include "md_HeartBeatREQ.h"
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
        BLOCK_id new_root_hash1;
        THASH_id attachment_hash;
        THASH_id tx_hash;
        REF_getter<HeartBeatREQ> heart_beat;
        size_t size()
        {
            size_t sz=0;
            sz+=new_root_hash1.container.size();
            sz+=attachment_hash.container.size();
            sz+=tx_hash.container.size();
            if(heart_beat.valid())
            sz+=heart_beat->size();
            return sz;
        }
        void dump(nlohmann::json& j)
        {
            j["new_root_hash1"]=new_root_hash1.str();
            j["attachment_hash"]=attachment_hash.str();
            j["tx_hash"]=tx_hash.str();
            
            if(heart_beat.valid())
                heart_beat->dump(j["heart_beat"]);
            
        }
        void update(Blake2bHasher& h) const
        {
            h.update(new_root_hash1.container);
            h.update(attachment_hash.container);
            h.update(tx_hash.container);
            heart_beat->update(h);
        }

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            b<<new_root_hash1;
            b<<attachment_hash;
            b<<tx_hash;
            b<<heart_beat;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>new_root_hash1;
            b>>attachment_hash;
            b>>tx_hash;
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
