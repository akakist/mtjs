#pragma once
#include "md_Base.h"
#include "NODE_id.h"
namespace MsgData
{
    struct BlockAcceptedRSP: public Base
    {

        static Base* construct()
        {
            return new BlockAcceptedRSP();
        }
        BlockAcceptedRSP():Base(msgid::BlockAcceptedRSP)
        {

        }
        NODE_id node_signer;
        BLOCK_id new_root_hash;
        blst_cpp::Signature sig_bls;
        void update(Blake2bHasher& h) const
        {
            h.update(node_signer.container);
            h.update(new_root_hash.container);
        }

        void sign(const blst_cpp::SecretKey& sk)
        {
            sig_bls.sign(sk, new_root_hash.container);
        }
        bool verify(const blst_cpp::PublicKey & pk) const
        {
            return sig_bls.verify(pk,new_root_hash.container);
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            b<<new_root_hash<<sig_bls<<node_signer;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>new_root_hash>>sig_bls>>node_signer;
        }
    };
}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::BlockAcceptedRSP> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::BlockAcceptedRSP> &s)
{
    auto ver=b.get_PN();
    s=new MsgData::BlockAcceptedRSP();
    s->unpack2(b);
    return b;
}
