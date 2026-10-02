#pragma once
#include "md_Base.h"

#include "md_BlockInfo.h"
#include "blst_cp.h"
namespace MsgData
{
    struct BlockValidatedREQ: public Base
    {

        BlockValidatedREQ();
        static Base* construct()
        {
            return new BlockValidatedREQ();
        }
        REF_getter<BlockInfo> blockInfo;
        std::vector<NODE_id> node_validators;
        blst_cpp::AggregateSignature agg_sig;
        void update(Blake2bHasher& h) const;
        void pack(outBuffer& b) const final;
        void unpack(inBuffer& b) final;
        size_t size()
        {
            size_t sz=0;
            if(blockInfo.valid())
                sz+blockInfo->size();
            
            for(auto &z: node_validators)
                sz+=z.container.size();
            sz+=agg_sig.serialize().size();

            return sz;
        }
    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::BlockValidatedREQ> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::BlockValidatedREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::BlockValidatedREQ();
    s->unpack2(b);
    return b;
}