#pragma once
#include "md_Base.h"
#include "md_LeaderCertificate.h"
namespace MsgData
{
    struct BlockAcceptedREQ: public Base
    {
        
        BlockAcceptedREQ();
        static Base* construct()
        {
            return new BlockAcceptedREQ();
        }
        REF_getter<LeaderCertificate> leader_certificateZ;
        REF_getter<BlockInfo> block_payload;
        std::vector<NODE_id> node_validators;
        blst_cpp::AggregateSignature agg_sig;
        void update(Blake2bHasher& h) const;
        void pack(outBuffer& b) const final;
        void unpack(inBuffer& b) final;
    };

}
