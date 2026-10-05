#pragma once
#include "md_Base.h"
#include "md_attachment_data.h"
#include "md_BlockAcceptedREQ.h"
#include "md_ValidateBlockREQ.h"
namespace MsgData
{
    struct BlockDBStore: public Base
    {

        BlockDBStore():Base(msgid::BlockDBStore),
            hb(new HeartBeatREQ()),
            blockAcceptedREQ(new BlockAcceptedREQ())

        {

        }
        // REF_getter<ValidateBlockREQ> validateBlockREQ_Z;
        std::vector<THASH_id> tx_hashes;
        REF_getter<HeartBeatREQ> hb;
        REF_getter<BlockAcceptedREQ> blockAcceptedREQ;
        size_t size(){
            size_t sz=0;
            for(auto &z: tx_hashes)
                sz+=z.container.size();
            if(hb.valid())
                sz+=hb->size();
            if(blockAcceptedREQ.valid())
                sz+=blockAcceptedREQ->size();
            return sz;
        }
        void update(Blake2bHasher& h) const
        {
            for(auto &z: tx_hashes)
                h.update(z.container);
            hb->update(h);
            blockAcceptedREQ->update(h);
        }
        void pack(outBuffer& b) const final
        {
            XTRY;
            MUTEX_INSPECTOR;
            Base::pack(b);
            b<<tx_hashes;
            b<<hb;
            b<<blockAcceptedREQ;
            XPASS;
        }
        void unpack(inBuffer& b) final
        {
            XTRY;
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>tx_hashes;
            b>>hb;
            b>>blockAcceptedREQ;
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
