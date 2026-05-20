#pragma once
#include <set>
#include "REF.h"
#include <optional>
#include <ioBuffer.h>
#include <blake2bHasher.h>
#include "tree.h"
#include "blst_cp.h"
#include "IUtils.h"
#include "BLOCK_id.h"
#include "TRANSACTION_id.h"
#include "THASH_id.h"
#include "s_ed.h"
#include "NODE_id.h"
#include "blst_cp.h"
#include "msgFactory.h"
extern thread_local MsgFactory msgFactory;
const char* msgName(int id);

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
struct transaction_report
{
    int err_code;
    std::string err_str;
    std::map<int, instruction_report> instruction_reports;
    void update(Blake2bHasher &h) const
    {
        h.update(std::to_string(err_code));
        h.update(err_str);
        for(auto& z: instruction_reports)
        {
            z.second.update(h);
        }
    }
};
inline outBuffer & operator<< (outBuffer& b,const transaction_report &s)
{
    b<<s.err_code<<s.err_str<<s.instruction_reports;
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  transaction_report &s)
{
    b>>s.err_code>>s.err_str>>s.instruction_reports;
    return b;
}

struct attachment_data
{
    std::vector<TRANSACTION_body> trs;
    std::map<THASH_id,transaction_report> transaction_reports;
    std::map<std::string,BigInt> fees;
    std::map<NODE_id,BigInt> rewards;
    void clear()
    {
        trs.clear();
        transaction_reports.clear();
        fees.clear();
        rewards.clear();
    }
    void hash(Blake2bHasher &h)
    {
        for(auto &z:trs)
        {
            h.update(z.container);
        }
        for(auto &z: transaction_reports)
        {
            z.second.update(h);
            // for(auto& y: z)
            // {
            //     y.update(h);
            // }
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
    }
};
inline outBuffer & operator<< (outBuffer& b,const attachment_data &s)
{
    b<<1;
    b<<s.trs<<s.transaction_reports<<s.fees<<s.rewards;
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  attachment_data &s)
{
    auto ver=b.get_PN();
    b>>s.trs>>s.transaction_reports>>s.fees>>s.rewards;
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
        BlockDBStore, GetSavedBlocksREQ,GetSavedBlocksRSP, DoHeartBeatREQ, ConfirmLeaderREQ, ConfirmLeaderRSP, InstructionList,TX,TxMint
    };

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

#ifdef KALL    
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
#endif

}

namespace MsgEvt
{
    struct Base: public Refcountable
    {
        int type;
        Base(int type_):Refcountable("MsgEvt::Base"),
        type(type_) {}
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
            outBuffer o;
            XTRY;
            MUTEX_INSPECTOR;
            pack(o);
            XPASS;
            return o.asString()->container;
        }
        virtual void hash(Blake2bHasher& h)=0;

    };


    struct HeartBeatREQ: public Base
    {
        
        HeartBeatREQ():Base(msgid::HeartBeatREQ)
        {

        }
        HeartBeatREQ(const BLOCK_id& _prev_block_hash, const BigInt& _newepoch, const NODE_id& _node_leader):Base(msgid::HeartBeatREQ),
        prev_block_hash(_prev_block_hash), new_epoch(_newepoch), node_leader(_node_leader)
        {
        }
        BLOCK_id prev_block_hash;
        BigInt new_epoch;
        NODE_id node_leader;
        void hash(Blake2bHasher& h)
        {
            h.update(prev_block_hash.container);
            h.update(new_epoch.toString());
            h.update(node_leader.container);
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            
            Base::pack(b);
            b<<prev_block_hash;
            b<<node_leader;
            b<<new_epoch;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>prev_block_hash;
            b>>node_leader;
            b>>new_epoch;
        }
        static Base* construct()
        {
            return new HeartBeatREQ();
        }

    };
    struct LeaderCertificate: public Base
    {
        
        LeaderCertificate():Base(msgid::LeaderCertificate), heart_beat(new MsgEvt::HeartBeatREQ())
        {

        }
        REF_getter<MsgEvt::HeartBeatREQ> heart_beat;
        std::vector<NODE_id> nodes;
        blst_cpp::AggregateSignature agg_sig;
        void hash(Blake2bHasher& h)
        {
            heart_beat->hash(h);
            for(auto& z: nodes)
            {
                h.update(z.container);
            }
        }

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
        
        GetTransactionREQ():Base(msgid::GetTransactionREQ),lc(new LeaderCertificate())
        {
        }
        REF_getter<LeaderCertificate> lc;
        void hash(Blake2bHasher& h)
        {
            lc->hash(h);
        }

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            lc->pack(b);
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            lc->unpack2(b);
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
        void hash(Blake2bHasher& h)
        {
            leader_cert->hash(h);
            for(auto& z: transaction_bodies)
            {
                h.update(z.container);
            }
        }

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
        void hash(Blake2bHasher& h);
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
        void hash(Blake2bHasher& h)
        {
            payload_heart_beat->hash(h);
            h.update(node_signer.container);
        }
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
        void hash(Blake2bHasher& h)
        {
            for(auto& z:trs )
                h.update(z.container);
        }

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
        void hash(Blake2bHasher& h)
        {
            h.update(prev_epoch.toString());
            h.update(prev_root_hash.container);
            h.update(new_root_hash1.container);
            h.update(attachment_hash.container);
            payload_heart_beat->hash(h);
        }

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
        void hash(Blake2bHasher& h)
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
        void hash(Blake2bHasher& h)
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
    struct GetSavedBlocksREQ: public Base
    {
        
        GetSavedBlocksREQ():Base(msgid::GetSavedBlocksREQ)
        {

        }
        static Base* construct()
        {
            return new GetSavedBlocksREQ();
        }
        BigInt epoch;
        void hash(Blake2bHasher& h)
        {
            h.update(epoch.toString());
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            b<<epoch;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>epoch;
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
        void hash(Blake2bHasher& h)
        {
            h.update(epoch.toString());
            att_data.hash(h);
            block_accepted_req->hash(h);
        }
        void pack(outBuffer& b) const final
        {
            XTRY;
            MUTEX_INSPECTOR;
            Base::pack(b);
            b<<epoch;
            b<<att_data;
            block_accepted_req->pack(b);
            XPASS;
        }
        void unpack(inBuffer& b) final
        {
            XTRY;
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>epoch;
            b>>att_data;
            block_accepted_req->unpack2(b);
            XPASS;
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
        std::vector<std::pair<BigInt, REF_getter<MsgEvt::BlockDBStore>> > blocks_Z;
        BigInt lastEpoch;
        void hash(Blake2bHasher& h)
        {
            throw CommonError("unimp");
        }
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
                REF_getter<MsgEvt::BlockDBStore> block_db_store(new MsgEvt::BlockDBStore());
                block_db_store->unpack2(b);
                blocks_Z.emplace_back(epoch, block_db_store);
            }
            b>>lastEpoch;
        }
    };
    struct DoHeartBeatREQ: public Base
    {
        
        static Base* construct()
        {
            return new DoHeartBeatREQ();
        }
        DoHeartBeatREQ():Base(msgid::DoHeartBeatREQ),prev_leader_cert(new LeaderCertificate)
        {

        }
        REF_getter<LeaderCertificate> prev_leader_cert;
        void hash(Blake2bHasher& h)
        {
            throw CommonError("unimp");
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            prev_leader_cert->pack(b);
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            prev_leader_cert->unpack2(b);
        }
    };
    struct ConfirmLeaderREQ: public Base
    {
        
        static Base* construct()
        {
            return new ConfirmLeaderREQ();
        }
        ConfirmLeaderREQ():Base(msgid::ConfirmLeaderREQ), hb(new HeartBeatREQ)
        {

        }
        REF_getter<HeartBeatREQ> hb;
        void hash(Blake2bHasher& h)
        {
            throw CommonError("unimp");
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            hb->pack(b);
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            hb->unpack2(b);
        }
    };
    struct ConfirmLeaderRSP: public Base
    {
        
        static Base* construct()
        {
            return new ConfirmLeaderRSP();
        }
        ConfirmLeaderRSP():Base(msgid::ConfirmLeaderRSP), hb(new HeartBeatREQ)
        {

        }
        REF_getter<HeartBeatREQ> hb;
        blst_cpp::Signature sig;        
        NODE_id node_signer;
        void hash(Blake2bHasher& h)
        {
            throw CommonError("unimp");
        }

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            hb->pack(b);
            b<<sig<<node_signer;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            hb->unpack2(b);
            b>>sig>>node_signer;
        }
    };
    struct InstructionList: public Base
    {
        
        static Base* construct()
        {
            return new InstructionList();
        }
        InstructionList():Base(msgid::InstructionList)
        {

        }
        std::vector<REF_getter<MsgEvt::Base>> instructions;

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            b<<instructions.size();
            for(auto& z: instructions)
            {
                z->pack(b);
            }
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            auto n=b.get_PN();
            for(int i=0;i<n;i++)
            {
                auto m=b.get_PN();
                REF_getter<MsgEvt::Base> msg=msgFactory.create(m);
                msg->unpack(b);
                instructions.push_back(msg);

            }
        }
        void hash(Blake2bHasher& h)
        {
            for(auto& z: instructions)
            {
                z->hash(h);
                // h.update(z);
            }
        }
    };
    struct TX: public Base
    {
        
        static Base* construct()
        {
            return new TX();
        }
        TX():Base(msgid::TX), instructions(new InstructionList())
        {

        }
        REF_getter<InstructionList> instructions;
        std::string user_pk_ed;
        std::string sig_ed;
        BigInt nonce;
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            instructions->pack(b);
            b<<user_pk_ed<<sig_ed<<nonce;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            instructions->unpack2(b);
            b>>user_pk_ed>>sig_ed>>nonce;
        }
        void hash(Blake2bHasher &h)
        {
            instructions->hash(h);
            h.update(user_pk_ed);
            h.update(nonce.toString());
        }
        void sign(const std::string& sk)
        {
            Blake2bHasher h;
            hash(h);
            sig_ed=sign_ed(sk,h.final());
        }
        bool verify()
        {
            Blake2bHasher h;
            hash(h);
            return verify_ed_pk(user_pk_ed,sig_ed,h.final());
        }
    };

    struct TxMint: public Base
    {
        TxMint():Base(msgid::TxMint) {}
        BigInt amount;
        static Base* construct()
        {
            return new TxMint();
        }

        void hash(Blake2bHasher &h)
        {
            h.update(amount.toString());
        }

        void pack(outBuffer& b) const final
        {
            Base::pack(b);
            b<<amount;
        }
        void unpack(inBuffer& b) final
        {
            Base::unpack(b);
            b>>amount;
        }
    };


}



