#pragma once
#include <set>
#include "REF.h"
#include <optional>
#include <ioBuffer.h>
#include <blake2bHasher.h>
#include "tree.h"
#include "blst_cp.h"
#include "IUtils.h"
#include "bls_io.h"
#include "BLOCK_id.h"
#include "TRANSACTION_id.h"
#include "THASH_id.h"
#include "s_ed.h"
#include "NODE_id.h"
#include "blst_cp.h"
struct instruction_report
{
    int err_code;
    std::string err_str;
    std::vector<std::string>  logMsgs;
    void update(Blake2bHasher &h) const
    {
        h.update(std::to_string(err_code));
        h.update(err_str);
        for(auto &s: logMsgs)
            h.update(s);
    }
};

inline outBuffer & operator<< (outBuffer& b,const instruction_report &s)
{
    b<<s.err_code<<s.err_str<<s.logMsgs;
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  instruction_report &s)
{
    b>>s.err_code>>s.err_str>>s.logMsgs;
    return b;
}
struct attachment_data
{
    std::vector<TRANSACTION_body> trs;
    std::vector<std::vector<instruction_report>> instruction_reports;
    std::map<std::string,BigInt> fees;
    std::map<NODE_id,BigInt> rewards;
    void clear()
    {
        trs.clear();
        instruction_reports.clear();
        fees.clear();
        rewards.clear();
    }
    std::string hash()
    {
        Blake2bHasher h;
        for(auto &z:trs)
        {
            h.update(z.container);
        }
        for(auto &z: instruction_reports)
        {
            for(auto& y: z)
            {
                y.update(h);
            }
        }
        for(auto &z: fees)
        {
            h.update(z.first);
            h.update(z.second.toString());
        }
        for(auto &z: rewards)
        {
            h.update(z.first.container);
            h.update(z.second.toString());
        }
        return h.final();
    }
};
inline outBuffer & operator<< (outBuffer& b,const attachment_data &s)
{
    b<<1;
    b<<s.trs<<s.instruction_reports<<s.fees<<s.rewards;
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  attachment_data &s)
{
    auto ver=b.get_PN();
    b>>s.trs>>s.instruction_reports>>s.fees>>s.rewards;
    return b;
}

namespace msgid
{
    enum MSG_ID
    {
        node_message_ed,
        user_message_req,transaction_added_rsp,
        user_request,get_user_status_req,get_user_status_rsp, HeartBeatREQ,HeartBeatRSP,
        LeaderCertificate, ValidateBlockREQ, ValidateBlockRSP, BlockInfo, BlockAcceptedREQ,BlockAcceptedRSP, GetTransactionREQ,GetTransactionRSP,
        BlockDBStore, GetSavedBlocksREQ,GetSavedBlocksRSP
    };

}
inline const char* msgName(int id)
{
    switch(id)
    {
    case msgid::node_message_ed:
        return "node_message_ed";
    case msgid::user_message_req:
        return "user_message_req";
    case msgid::transaction_added_rsp:
        return "transaction_added_rsp";
    case msgid::user_request:
        return "user_request";
    case msgid::get_user_status_req:
        return "get_user_status_req";
    case msgid::get_user_status_rsp:
        return "get_user_status_rsp";
    case msgid::HeartBeatREQ:
        return "HeartBeatREQ";
    case msgid::HeartBeatRSP:
        return "HeartBeatRSP";
    case msgid::LeaderCertificate:
        return "LeaderCertificate";
    case msgid::ValidateBlockREQ:
        return "ValidateBlockREQ";
    case msgid::ValidateBlockRSP:
        return "ValidateBlockRSP";
    case msgid::BlockInfo:
        return "BlockInfo";
    case msgid::BlockAcceptedREQ:
        return "BlockAcceptedREQ";
    case msgid::BlockAcceptedRSP:
        return "BlockAcceptedRSP";
    case msgid::GetTransactionREQ:
        return "GetTransactionREQ";
    case msgid::GetTransactionRSP:
        return "GetTransactionRSP";
    case msgid::BlockDBStore:
        return "BlockDBStore";
    case msgid::GetSavedBlocksREQ:
        return "GetSavedBlocksREQ";
    case msgid::GetSavedBlocksRSP:
        return "GetSavedBlocksRSP";
    default:
        return "unknown";
    }
}
namespace msg
{
    struct message_base
    {
        int type;
        message_base(int type_):type(type_) {}
        virtual ~message_base() {}
        virtual void pack(outBuffer& b) const
        {
            MUTEX_INSPECTOR;
            // std::string signature;
            b<<type;

        }
        virtual void unpack(inBuffer& b)
        {
            // std::string signature;
            // b>>signature;
        }
        std::string getBuffer() const
        {
            MUTEX_INSPECTOR;
            outBuffer o;
            pack(o);
            return o.asString()->container;
        }

    };

    struct user_message_req: public message_base
    {
        user_message_req():message_base(msgid::user_message_req) {}
        user_message_req(inBuffer &in):message_base(msgid::user_message_req) {
            unpack(in);
        }
        user_message_req(const TRANSACTION_body& s):message_base(msgid::user_message_req) {
            inBuffer in(s.container);
            int t=in.get_PN();
            if(t!=msgid::user_message_req)
                throw CommonError("if(t!=msgid::user_message_req)");
            unpack(in);
        }
        std::vector<std::string> payload;
        BigInt nonce;
        std::string signature;
        std::string address_pk_ed;
        bool verify()
        {
            Blake2bHasher h;
            for(auto& z:payload)
                h.update(z);
            h.update(nonce.toString());
            return verify_ed_pk(address_pk_ed,signature,h.final());
        }
        void sign(const std::string &sk)
        {
            Blake2bHasher h;
            for(auto& z:payload)
                h.update(z);
            h.update(nonce.toString());
            signature=sign_ed(sk,h.final());
            // pk.resize(crypto_sign_PUBLICKEYBYTES);
            // crypto_sign_ed25519_sk_to_pk((uint8_t*)pk.data(), (unsigned char*)sk.data());

        }
        void pack(outBuffer& b) const final
        {

            message_base::pack(b);
            b<<payload;
            b<<nonce;
            b<< signature<<address_pk_ed;

        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            message_base::unpack(b);
            b>>payload;
            b>>nonce;
            b>> signature>>address_pk_ed;
        }
    };
    struct transaction_added_rsp: public message_base
    {
        transaction_added_rsp():message_base(msgid::transaction_added_rsp) {}
        int err;
        std::string err_str;
        THASH_id tx_hash;
        void pack(outBuffer& b) const final
        {

            message_base::pack(b);
            b<<err;
            b<<err_str;
            b<<tx_hash;

        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            message_base::unpack(b);
            b>>err;
            b>>err_str;
            b>>tx_hash;
        }
    };

    struct user_request: public message_base
    {
        user_request():message_base(msgid::user_request) {}
        user_request(inBuffer& in):message_base(msgid::user_request)
        {
            unpack(in);
        }
        std::string payload;
        std::string rnd;

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            message_base::pack(b);
            b<<payload<<rnd;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            message_base::unpack(b);
            b>>payload>>rnd;
        }

    };
    struct get_user_status_req: public message_base
    {
        get_user_status_req():message_base(msgid::get_user_status_req)
        {

        }
        get_user_status_req(inBuffer &in):message_base(msgid::get_user_status_req)
        {
            unpack(in);
        }
        std::string address_pk_ed;
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            message_base::pack(b);
            b<<address_pk_ed;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            message_base::unpack(b);
            b>>address_pk_ed;
        }
    };

    struct get_user_status_rsp: public message_base
    {
        std::string address_pk_ed;
        BigInt nonce;
        BigInt balance;
        get_user_status_rsp():message_base(msgid::get_user_status_rsp)
        {

        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            message_base::pack(b);
            b<<address_pk_ed<<nonce<<balance;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            message_base::unpack(b);
            b>>address_pk_ed>>nonce>>balance;
        }

    };
    struct node_message_ed: public message_base
    {

        node_message_ed():message_base(msgid::node_message_ed) {}
        node_message_ed(inBuffer& in):message_base(msgid::node_message_ed)
        {
            unpack(in);
        }
        node_message_ed(const std::string _payload, const NODE_id & _src_node, const std::string& sk ):message_base(msgid::node_message_ed),
            payload(_payload),src_node(_src_node)
        {
            signature=sign_ed(sk,blake2b_hash(_payload).container);
        }
        bool verify(const std::string & pk)
        {
            auto res=verify_ed_pk(pk,signature,blake2b_hash(payload));
            return res;
        }
        std::string payload;
        NODE_id src_node;
        std::string signature;
        void pack(outBuffer& b) const final
        {
            message_base::pack(b);
            b<<payload<<src_node<<signature;
        }
        void unpack(inBuffer& b) final
        {
            message_base::unpack(b);
            b>>payload>>src_node >> signature;
        }

    };


}

namespace MsgEvent
{
    struct Base: public Refcountable
    {
        int type;
        Base(int type_):type(type_) {}
        virtual ~Base() {}
        virtual void pack(outBuffer& b) const
        {
            MUTEX_INSPECTOR;
            b<<type;

        }
        virtual void unpack(inBuffer& b)
        {
        }
        void unpack2(inBuffer& b)
        {
            MUTEX_INSPECTOR;
            auto t=b.get_PN();
            if(t!=type)
                throw CommonError("if(type!=mtype)");
            unpack(b);
        }
        std::string getBuffer() const
        {
            MUTEX_INSPECTOR;
            outBuffer o;
            pack(o);
            return o.asString()->container;
        }

    };
    struct HeartBeatREQ: public Base
    {
        HeartBeatREQ():Base(msgid::HeartBeatREQ)
        {

        }
        HeartBeatREQ(const BLOCK_id& _prev_block_hash, const BigInt& _epoch, const NODE_id& _node_leader):Base(msgid::HeartBeatREQ),
        prev_block_hash(_prev_block_hash), epoch(_epoch), node_leader(_node_leader)
        {
        }
        BLOCK_id prev_block_hash;
        BigInt epoch;
        NODE_id node_leader;
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            
            Base::pack(b);
            b<<prev_block_hash;
            b<<node_leader;
            b<<epoch;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>prev_block_hash;
            b>>node_leader;
            b>>epoch;
        }
        static Base* construct()
        {
            return new HeartBeatREQ();
        }

    };
    struct LeaderCertificate: public Base
    {
        LeaderCertificate():Base(msgid::LeaderCertificate), heart_beat(new MsgEvent::HeartBeatREQ())
        {

        }
        REF_getter<MsgEvent::HeartBeatREQ> heart_beat;
        std::vector<NODE_id> nodes;
        blst_cpp::AggregateSignature agg_sig;
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            heart_beat->pack(b);
            // b<<payload_heart_beat;
            b<<nodes;
            b<<agg_sig;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            // auto t=b.get_PN();
            // if(t!=msgid::HeartBeatREQ)                throw CommonError("if(t!=msgid::HeartBeatREQ)");
            heart_beat->unpack2(b);
            // b>>payload_heart_beat;
            b>>nodes;
            b>>agg_sig;
        }
        static Base* construct()
        {
            return new LeaderCertificate();
        }

    };
    struct GetTransactionREQ: public Base
    {
        GetTransactionREQ():Base(msgid::GetTransactionREQ),payload_lc(new LeaderCertificate())
        {
        }
        REF_getter<LeaderCertificate> payload_lc;
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            payload_lc->pack(b);
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            payload_lc->unpack2(b);
        }

        static Base* construct()
        {
            return new GetTransactionREQ();
        }
    };
    struct ValidateBlockREQ: public Base
    {
        ValidateBlockREQ():Base(msgid::ValidateBlockREQ),leader_cert(new LeaderCertificate())
        {
        }
        static Base* construct()
        {
            return new ValidateBlockREQ();
        }

        REF_getter<LeaderCertificate>  leader_cert;
        std::vector<TRANSACTION_body> transaction_bodies;
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            leader_cert->pack(b);
            b<<transaction_bodies;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            leader_cert->unpack2(b);
            b>>transaction_bodies;
        }

    };
    struct BlockInfo;
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
        void pack(outBuffer& b) const final;
        void unpack(inBuffer& b) final;
    };
    struct HeartBeatRSP: public Base
    {
        HeartBeatRSP():Base(msgid::HeartBeatRSP), payload_heart_beat(new HeartBeatREQ())
        {
        }
        REF_getter<HeartBeatREQ> payload_heart_beat;
        NODE_id node_signer;
        blst_cpp::Signature signature;
        static Base* construct()
        {
            return new HeartBeatRSP();
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            payload_heart_beat->pack(b);
            b<<node_signer;
            b<<signature;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            payload_heart_beat->unpack2(b);
            b>>node_signer;
            b>>signature;
        }
    };
    struct GetTransactionRSP: public Base
    {
        static Base* construct()
        {
            return new GetTransactionRSP();
        }
        GetTransactionRSP():Base(msgid::GetTransactionRSP)
        {
            
        }
        GetTransactionRSP(inBuffer &in):Base(msgid::GetTransactionRSP)
        {
            unpack(in);
        }

        std::vector<TRANSACTION_body>  trs;
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            b<<trs;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>trs;
        }

    };
    struct BlockInfo: public Base
    {
        static Base* construct()
        {
            return new BlockInfo();
        }
        BlockInfo():Base(msgid::BlockInfo),payload_heart_beat(new HeartBeatREQ())
        {

        }
        BigInt prev_epoch;
        BLOCK_id prev_root_hash;
        BLOCK_id new_root_hash1;
        THASH_id attachment_hash;
        REF_getter<HeartBeatREQ> payload_heart_beat;
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            b<<prev_epoch;
            b<<prev_root_hash;
            b<<new_root_hash1;
            b<<attachment_hash;
            payload_heart_beat->pack(b);
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>prev_epoch;
            b>>prev_root_hash;
            b>>new_root_hash1;
            b>>attachment_hash;
            payload_heart_beat->unpack2(b);
        }

    };
    
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
    struct GetSavedBlocksREQ: public Base
    {
        GetSavedBlocksREQ():Base(msgid::GetSavedBlocksREQ)
        {

        }
        static Base* construct()
        {
            return new GetSavedBlocksREQ();
        }
        BigInt myEpoch;
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            b<<myEpoch;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>myEpoch;
        }
    };
    struct BlockDBStore: public Base
    {
        BlockDBStore():Base(msgid::BlockDBStore), block_accepted_req(new BlockAcceptedREQ())
        {

        }
        BigInt epoch;
        attachment_data att_data;
        REF_getter<BlockAcceptedREQ> block_accepted_req;
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            b<<epoch;
            b<<att_data;
            block_accepted_req->pack(b);
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>epoch;
            b>>att_data;
            block_accepted_req->unpack2(b);
        }

    };
    struct GetSavedBlocksRSP: public Base
    {
        static Base* construct()
        {
            return new GetSavedBlocksRSP();
        }
        GetSavedBlocksRSP():Base(msgid::GetSavedBlocksRSP)
        {

        }
        std::vector<std::pair<BigInt, REF_getter<MsgEvent::BlockDBStore>> > blocks_Z;
        BigInt lastEpoch;
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            b<<blocks_Z.size();
            for(auto &z: blocks_Z)
            {
                b<<z.first;
                z.second->pack(b);
            }
            b<<lastEpoch;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            int size=b.get_PN();
            for(int i=0;i<size;i++)            {
                BigInt epoch;
                b>>epoch;
                REF_getter<MsgEvent::BlockDBStore> block_db_store(new MsgEvent::BlockDBStore());
                block_db_store->unpack2(b);
                blocks_Z.emplace_back(epoch, block_db_store);
            }
            b>>lastEpoch;
        }
    };


}



class MsgFactory {
public:
    using Constructor = MsgEvent::Base* (*)();
    
    MsgEvent::Base* create(const int& id) {

        auto it = registry.find(id);
        if(it==registry.end())
        {
            throw CommonError("Message type not found %d %s", id,msgName(id));
        }
        return it->second();
    }
    
    bool registerMsg(const int& id, Constructor ctor) {
        registry[id] = ctor;
        return true;
    }
    
private:
        std::map<int, Constructor> registry;
};

