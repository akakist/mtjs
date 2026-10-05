#pragma once
#include "md_Base.h"

// #include "md_BlockInfo.h"
#include "md_BlockAcceptedREQ.h"
#include "blst_cp.h"
namespace MsgData
{
    struct BlockDiffValidateREQ: public Base
    {

        BlockDiffValidateREQ(): Base(msgid::BlockDiffValidateREQ), blockAcceptedREQ(new BlockAcceptedREQ)
        {

        }
        static Base* construct()
        {
            return new BlockDiffValidateREQ();
        }
        std::map<std::string,std::string> diffs;
        REF_getter<BlockAcceptedREQ> blockAcceptedREQ;
        // std::vector<NODE_id> node_validators;
        // blst_cpp::AggregateSignature agg_sig;
        void update(Blake2bHasher& h) const
        {
    MUTEX_INSPECTOR;
            blockAcceptedREQ->update(h);
            for(auto& z: diffs)
            {
                h.update(z.first);
                h.update(z.second);
            }
        }
        void pack(outBuffer& b) const final
        {
    MUTEX_INSPECTOR;
            Base::pack(b);
            b<<diffs;
            b<<blockAcceptedREQ;
        }
        void unpack(inBuffer& b) final
        {
    MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>diffs;
            b>>blockAcceptedREQ;
        }
        size_t size()
        {
            size_t sz=0;
            if(blockAcceptedREQ.valid())
                sz+=blockAcceptedREQ->size();

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