#pragma once
#include "md_Base.h"
namespace MsgData
{
    struct ValidateBlockRSP: public Base
    {
        
        static Base* construct()
        {
            return new ValidateBlockRSP();
        }
        ValidateBlockRSP(): Base(msgid::ValidateBlockRSP), payload_block(new BlockInfo())
        {

        }
        REF_getter<BlockInfo> payload_block;
        blst_cpp::Signature sig;
        NODE_id node_validator;
        void update(Blake2bHasher& h) const
        {
            payload_block->hash(h);
            h.update(node_validator.container);
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            payload_block->pack(b);
            b<<sig;
            b<<node_validator;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            payload_block->unpack2(b);
            b>>sig;
            b>>node_validator;
        }
        void sign(const blst_cpp::SecretKey &sk)
        {
            // Blake2bHasher h;
            // h.update(payload_block);
            sig.sign(sk, blake2b_hash(payload_block->getBuffer()).container);
        }
        bool verify(const blst_cpp::PublicKey &pk) const
        {
            return sig.verify(pk, blake2b_hash(payload_block->getBuffer()).container);
        }

    };

}
