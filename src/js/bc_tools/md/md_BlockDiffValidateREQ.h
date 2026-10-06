#pragma once
#include "md_Base.h"

// #include "md_BlockInfo.h"
#include "md_BlockAcceptedREQ.h"
#include "blst_cp.h"
#include "md_attachment_data.h"
#include "md_BlockDBStore.h"
namespace MsgData
{
    struct BlockDiffValidateREQ: public Base
    {

        BlockDiffValidateREQ(): Base(msgid::BlockDiffValidateREQ), blockDBStore(new BlockDBStore)
        {

        }
        static Base* construct()
        {
            return new BlockDiffValidateREQ();
        }
        // REFBlockStore
        REF_getter<BlockDBStore> blockDBStore;
        // std::map<std::string,std::string> diffs;
        // REF_getter<BlockAcceptedREQ> blockAcceptedREQ;
        // std::vector<THASH_id> tx_hashes;
        // REF_getter<attachment_data> att_data;

        // std::vector<NODE_id> node_validators;
        // blst_cpp::AggregateSignature agg_sig;
        void update(Blake2bHasher& h) const
        {
    MUTEX_INSPECTOR;
            blockDBStore->update(h);
            // for(auto& z: diffs)
            // {
            //     h.update(z.first);
            //     h.update(z.second);
            // }
            // for(auto& z: tx_hashes)
            // {
            //     h.update(z.container);
            // }
            // att_data->update(h);
        }
        void pack(outBuffer& b) const final
        {
    MUTEX_INSPECTOR;
            Base::pack(b);
            b<<blockDBStore;
            // b<<diffs;
            // b<<blockAcceptedREQ;
            // b<<tx_hashes;
            // b<<att_data;
        }
        void unpack(inBuffer& b) final
        {
    MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>blockDBStore;
            // b>>diffs;
            // b>>blockAcceptedREQ;
            // b>>tx_hashes;
            // b>>att_data;
        }
        size_t size()
        {
            size_t sz=0;
            if(blockDBStore.valid())
                sz+=blockDBStore->size();

            // for(auto& z: diffs)
            // {
            //     sz+=z.first.size();
            //     sz+=z.second.size();
            // }
            // for(auto& z: tx_hashes)
            // {
            //     sz+=z.container.size();
            // }
            // sz+=att_data->size();

            return sz;
        }
    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::BlockDiffValidateREQ> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::BlockDiffValidateREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::BlockDiffValidateREQ();
    s->unpack2(b);
    return b;
}