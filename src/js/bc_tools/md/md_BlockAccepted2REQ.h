#pragma once
#include <set>
#include "md_Base.h"

#include "md_BlockInfo.h"
#include "blst_cp.h"
#include "md_BlockAcceptedREQ.h"
namespace MsgData
{
    struct BlockAccepted2REQ: public Base
    {

        BlockAccepted2REQ(): Base(msgid::BlockAccepted2REQ), blockAcceptedREQ(new BlockAcceptedREQ)
        {
            
        }
        static Base* construct()
        {
            return new BlockAccepted2REQ();
        }
        REF_getter<BlockAcceptedREQ> blockAcceptedREQ;
        std::set<NODE_id> node_diff_validators;
        blst_cpp::AggregateSignature agg_diff_sig;
        void update(Blake2bHasher& h) const
        {
            blockAcceptedREQ->update(h);
            for(auto& z: node_diff_validators)
            {
                h.update(z.container);
            }
            if (agg_diff_sig.initialized())
                h.update(agg_diff_sig.serialize());
        }
        void pack(outBuffer& b) const final
        {
            Base::pack(b);
            b<<blockAcceptedREQ;
            b<<node_diff_validators;
            if (agg_diff_sig.initialized()) b<<agg_diff_sig; else b<<std::string();
        }
        void unpack(inBuffer& b) final
        {
            Base::unpack(b);
            b>>blockAcceptedREQ;
            b>>node_diff_validators;
            std::string s;
            b>>s;
            if (!s.empty()) agg_diff_sig.deserialize(s);
        }
    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::BlockAccepted2REQ> &s)
{
    MUTEX_INSPECTOR;
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::BlockAccepted2REQ> &s)
{
    MUTEX_INSPECTOR;
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::BlockAccepted2REQ();
    s->unpack2(b);
    return b;
}