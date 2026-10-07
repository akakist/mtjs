#pragma once
#include "md_Base.h"

#include "md_BlockInfo.h"
#include "md_BlockAcceptedREQ.h"
#include "md_TX.h"
#include "blst_cp.h"
#include "NODE_id.h"
#include "md_BlockDBStore.h"
#include "md_attachment_data.h"
namespace MsgData
{
    struct ValidateBlockRSP: public Base
    {

        static Base* construct()
        {
            return new ValidateBlockRSP();
        }
        ValidateBlockRSP(): Base(msgid::ValidateBlockRSP), blockInfo(new BlockInfo())
        {

        }
        REF_getter<BlockInfo> blockInfo;
        std::vector<THASH_id> tx_hashes;
        REF_getter<attachment_data> att_data;
        std::map<std::string,std::string> diffs;
        blst_cpp::Signature sig;
        NODE_id node_validator;
        void update(Blake2bHasher& h) const
        {
            blockInfo->update(h);
            h.update(sig.serialize());
            h.update(node_validator.container);
            for(auto& t: tx_hashes)
            {
                h.update(t.container);
            }
            for(auto& t:diffs)
            {
                h.update(t.first);
                h.update(t.second);

            }
            att_data->update(h);
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            b<<blockInfo;
            b<<sig;
            b<<node_validator;
            b<<tx_hashes<<att_data<<diffs;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>blockInfo;
            b>>sig;
            b>>node_validator;
            b>>tx_hashes>>att_data>>diffs;
        }
        void sign(const blst_cpp::SecretKey &sk)
        {
            sig.sign(sk, blockInfo->getHash().container);
        }
        bool verify(const blst_cpp::PublicKey &pk) const
        {
            return sig.verify(pk, blockInfo->getHash().container);
        }

    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::ValidateBlockRSP> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::ValidateBlockRSP> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::ValidateBlockRSP();
    s->unpack2(b);
    return b;
}
