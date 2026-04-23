#pragma once
#include "msg.h"
namespace tx_id
{
    enum {
        registerNode,
        stake,unstake,transfer,createContract, mint, transferContract, registerUser

    };
}
inline const char* txName(int id)
{
    switch(id)
    {
    case tx_id::registerNode:
        return "registerNode";
    case tx_id::stake:
        return "stake";
    case tx_id::unstake:
        return "unstake";
    case tx_id::transfer:
        return "transfer";
    case tx_id::createContract:
        return "createContract";
    case tx_id::mint:
        return "mint";
    case tx_id::transferContract:
        return "transferContract";
    case tx_id::registerUser:
        return "registerUser";
    default:
        return "unknown";
    }
}
namespace tx
{
    struct registerUser: public msg::message_base
    {
        registerUser():message_base(tx_id::registerUser) {}
        std::string nickName;
        std::string pk;
        void pack(outBuffer& b) const final
        {
            message_base::pack(b);
            b<<nickName<<pk;
        }
        void unpack(inBuffer& b) final
        {
            message_base::unpack(b);
            b>>nickName>>pk;
        }

    };
    struct registerNode: public msg::message_base
    {
        registerNode():message_base(tx_id::registerNode) {}
        NODE_id name;
        std::string ip;
        std::string pk_ed;
        bls::PublicKey pk_bls;
        void pack(outBuffer& b) const final
        {
            message_base::pack(b);
            b<<name<<ip<<pk_ed<<pk_bls;
        }
        void unpack(inBuffer& b) final
        {
            message_base::unpack(b);
            b>>name>>ip>>pk_ed>>pk_bls;
        }

    };
    struct stake: public msg::message_base
    {
        stake():message_base(tx_id::stake) {}
        NODE_id node;
        BigInt amount;
        void pack(outBuffer& b) const final
        {
            message_base::pack(b);
            b<<node<<amount;
        }
        void unpack(inBuffer& b) final
        {
            message_base::unpack(b);
            b>>node>>amount;
        }
    };
    struct unstake: public msg::message_base
    {
        unstake():message_base(tx_id::unstake) {}
        NODE_id node;
        BigInt amount;
        void pack(outBuffer& b) const final
        {
            message_base::pack(b);
            b<<node<<amount;
        }
        void unpack(inBuffer& b) final
        {
            message_base::unpack(b);
            b>>node>>amount;
        }
    };
    struct transfer: public msg::message_base
    {
        transfer():message_base(tx_id::transfer) {}

        std::string to_nick;
        BigInt amount;
        void pack(outBuffer& b) const final
        {
            message_base::pack(b);
            b<<to_nick<<amount;
        }
        void unpack(inBuffer& b) final
        {
            message_base::unpack(b);
            b>>to_nick>>amount;
        }
    };
    struct createContract: public msg::message_base
    {
        createContract():message_base(tx_id::createContract) {}
        std::string name;
        std::string src;
        void pack(outBuffer& b) const final
        {
            message_base::pack(b);
            b<<name<<src;
        }
        void unpack(inBuffer& b) final
        {
            message_base::unpack(b);
            b>>name>>src;
        }
    };
    struct mint: public msg::message_base
    {
        mint():message_base(tx_id::mint) {}
        BigInt amount;
        void pack(outBuffer& b) const final
        {
            message_base::pack(b);
            b<<amount;
        }
        void unpack(inBuffer& b) final
        {
            message_base::unpack(b);
            b>>amount;
        }
    };
    struct transferContract: public msg::message_base
    {
        transferContract():message_base(tx_id::transferContract) {}
        std::string to;
        bool by_nick;
        std::string contract;
        void pack(outBuffer& b) const final
        {
            message_base::pack(b);
            b<<to<<by_nick<<contract;
        }
        void unpack(inBuffer& b) final
        {
            message_base::unpack(b);
            b>>to>>by_nick>>contract;
        }
    };

};
