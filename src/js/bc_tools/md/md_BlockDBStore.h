#pragma once
#include "md_Base.h"
#include "md_attachment_data.h"
#include "md_BlockAccepted2REQ.h"
#include "md_ValidateBlockREQ.h"
namespace MsgData
{
    struct BlockDBStore: public Base
    {

        BlockDBStore():Base(msgid::BlockDBStore),
            // hb(new HeartBeatREQ()),
            blockAccepted2REQ(new BlockAccepted2REQ())

        {

        }
        static Base* construct()
        {
            return new BlockDBStore();
        }

        // REF_getter<ValidateBlockREQ> validateBlockREQ_Z;
        // REF_getter<HeartBeatREQ> hb;
        REF_getter<BlockAccepted2REQ> blockAccepted2REQ;

        std::vector<THASH_id> tx_hashes;
        std::map<std::string, std::string> diffs;
        REF_getter<MsgData::attachment_data> att_data= nullptr;

        size_t size(){
            size_t sz=0;
            // for(auto &z: tx_hashes)
            //     sz+=z.container.size();
            // if(hb.valid())
            //     sz+=hb->size();
            if(blockAccepted2REQ.valid())
                sz+=blockAccepted2REQ->size();
            // if(att_data_Z.valid())
            //     sz+=att_data_Z->size();
            // for(auto &z: diffs_Z)
            // {
            //     sz+=z.first.size();
            //     sz+=z.second.size();
            // }
            return sz;
        }
        void update(Blake2bHasher& h) const
        {
            // for(auto &z: tx_hashes)
            //     h.update(z.container);
            // hb->update(h);
            blockAccepted2REQ->update(h);
            for(auto& z: tx_hashes)
            {
                h.update(z.container);
            }
            for(auto& z: diffs)
            {
                h.update(z.first);
                h.update(z.second);
            }
            att_data->update(h);
        }
        void pack(outBuffer& b) const final
        {
            XTRY;
            MUTEX_INSPECTOR;
            Base::pack(b);
            // b<<hb;
            b<<blockAccepted2REQ;
            b<<tx_hashes;
            b<<diffs;
            b<<att_data;
            XPASS;
        }
        void unpack(inBuffer& b) final
        {
            XTRY;
            MUTEX_INSPECTOR;
            Base::unpack(b);
            // b>>hb;
            b>>blockAccepted2REQ;
            b>>tx_hashes;
            b>>diffs;
            b>>att_data;
            XPASS;
        }

    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::BlockDBStore> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::BlockDBStore> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::BlockDBStore();
    s->unpack2(b);
    return b;
}
