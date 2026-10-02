#pragma once
#include "md_Base.h"

// #include "md_BlockInfo.h"
#include "md_BlockValidatedREQ.h"
#include "blst_cp.h"
namespace MsgData
{
    struct BlockDiffValidateREQ: public Base
    {

        BlockDiffValidateREQ(): Base(msgid::BlockDiffValidateREQ), blockInfo(new BlockInfo)
        {

        }
        static Base* construct()
        {
            return new BlockDiffValidateREQ();
        }
        REF_getter<BlockInfo> blockInfo;
        std::map<std::string,std::string> diffs;
        // std::vector<NODE_id> node_validators;
        // blst_cpp::AggregateSignature agg_sig;
        void update(Blake2bHasher& h) const
        {
            blockInfo->update(h);
            for(auto& z: diffs)
            {
                h.update(z.first);
                h.update(z.second);
            }
        }
        void pack(outBuffer& b) const final
        {
            Base::pack(b);
            blockInfo->pack(b);
            b<<diffs;
        }
        void unpack(inBuffer& b) final
        {
            Base::unpack(b);
            blockInfo->unpack(b);
            b>>diffs;
        }
        size_t size()
        {
            size_t sz=0;
            if(blockInfo.valid())
                sz=+blockInfo->size();

            for(auto& z: diffs)
            {
                sz+=z.first.size();
                sz+=z.second.size();
            }
            // for(auto &z: node_validators)
            //     sz+=z.container.size();
            // sz+=agg_sig.serialize().size();

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