#pragma once
#include "md_Base.h"
#include "md_LeaderCertificate.h"
#include "md_BlockInfo.h"
namespace MsgData
{
    struct BlockAcceptedREQ: public Base
    {
        
        BlockAcceptedREQ();
        static Base* construct()
        {
            return new BlockAcceptedREQ();
        }
        // REF_getter<LeaderCertificate> leader_certificateZ;
        REF_getter<BlockInfo> blockInfo;
        std::vector<NODE_id> node_validators;
        blst_cpp::AggregateSignature agg_sig;
        void update(Blake2bHasher& h) const;
        void pack(outBuffer& b) const final;
        void unpack(inBuffer& b) final;
    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::BlockAcceptedREQ> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::BlockAcceptedREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::BlockAcceptedREQ();
    s->unpack2(b);
    return b;
}