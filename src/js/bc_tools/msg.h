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
        user_request,get_user_status_req,get_user_status_rsp, heart_beat,heart_beat_rsp,
        leader_certificate, block_request, block_response, blockZ, block_accepted_req,block_accepted_rsp, request_for_transactions,response_with_transactions,
        publish_block, get_blocks_req,get_blocks_rsp
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
    case msgid::heart_beat:
        return "heart_beat";
    case msgid::heart_beat_rsp:
        return "heart_beat_rsp";
    case msgid::leader_certificate:
        return "leader_certificate";
    case msgid::block_request:
        return "block_request";
    case msgid::block_response:
        return "block_response";
    case msgid::blockZ:
        return "blockZ";
    case msgid::block_accepted_req:
        return "block_accepted_req";
    case msgid::block_accepted_rsp:
        return "block_accepted_rsp";
    case msgid::request_for_transactions:
        return "request_for_transactions";
    case msgid::response_with_transactions:
        return "response_with_transactions";
    case msgid::publish_block:
        return "publish_block";
    case msgid::get_blocks_req:
        return "get_blocks_req";
    case msgid::get_blocks_rsp:
        return "get_blocks_rsp";
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
    struct heart_beat: public message_base
    {
        heart_beat():message_base(msgid::heart_beat)
        {

        }
        heart_beat(const std::string& s):message_base(msgid::heart_beat)
        {
            inBuffer in(s);
            auto t=in.get_PN();
            if(t!=msgid::heart_beat)
                throw CommonError("if(t!=msgid::heart_beat)");
            unpack(in);
        }
        BLOCK_id prev_block_hash;
        BigInt epoch;
        NODE_id node_leader;
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            message_base::pack(b);
            b<<prev_block_hash;
            b<<node_leader;
            b<<epoch;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            message_base::unpack(b);
            b>>prev_block_hash;
            b>>node_leader;
            b>>epoch;
        }
    };
    struct heart_beat_rsp: public message_base
    {
        heart_beat_rsp():message_base(msgid::heart_beat_rsp)
        {
        }
        std::string payload_heart_beat;
        NODE_id node_signer;
        blst_cpp::Signature signature;
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            message_base::pack(b);
            b<<payload_heart_beat;
            b<<node_signer;
            b<<signature;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            message_base::unpack(b);
            b>>payload_heart_beat;
            b>>node_signer;
            b>>signature;
        }
    };

// leader_certificate;
//
    struct request_for_transactions: public message_base
    {
        request_for_transactions():message_base(msgid::request_for_transactions)
        {

        }
        request_for_transactions(inBuffer & in):message_base(msgid::request_for_transactions)
        {
            unpack(in);
        }
        std::string payload_lc;
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            message_base::pack(b);
            b<<payload_lc;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            message_base::unpack(b);
            b>>payload_lc;
        }

    };
    struct response_with_transactions: public message_base
    {
        response_with_transactions():message_base(msgid::response_with_transactions)
        {

        }
        response_with_transactions(const std::string& s):message_base(msgid::response_with_transactions)
        {
            inBuffer in(s);
            auto t=in.get_PN();
            if(t!=msgid::response_with_transactions)
                throw CommonError("if(t!=msgid::response_with_transactions)");
            unpack(in);
        }
        response_with_transactions(inBuffer &in):message_base(msgid::response_with_transactions)
        {
            unpack(in);
        }

        std::vector<TRANSACTION_body>  trs;
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            message_base::pack(b);
            b<<trs;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            message_base::unpack(b);
            b>>trs;
        }

    };

    struct leader_certificate: public message_base
    {
        leader_certificate():message_base(msgid::leader_certificate)
        {

        }
        leader_certificate(const std::string& s):message_base(msgid::leader_certificate)
        {
            inBuffer in(s);
            auto t=in.get_PN();
            if(t!=msgid::leader_certificate)
                throw CommonError("if(t!=msgid::leader_certificate)");
            unpack(in);
        }
        std::string payload_heart_beat;
        std::vector<NODE_id> nodes;
        blst_cpp::AggregateSignature agg_sig;
        // BlsPublicKey agg_pk;
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            message_base::pack(b);
            b<<payload_heart_beat;
            b<<nodes;
            b<<agg_sig;
            // b<<agg_pk;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            message_base::unpack(b);
            b>>payload_heart_beat;
            b>>nodes;
            b>>agg_sig;
            // b>>agg_pk;
        }

    };
    struct block_request: public message_base
    {
        block_request():message_base(msgid::block_request)
        {
        }
        block_request(inBuffer& in):message_base(msgid::block_request)
        {
            unpack(in);
        }

        std::string leader_cert;
        std::vector<TRANSACTION_body> transaction_bodies;
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            message_base::pack(b);
            b<<leader_cert;
            b<<transaction_bodies;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            message_base::unpack(b);
            b>>leader_cert;
            b>>transaction_bodies;
        }

    };
    struct blockZ: public message_base
    {
        blockZ():message_base(msgid::blockZ)
        {

        }
        blockZ(const std::string& s):message_base(msgid::blockZ)
        {
            inBuffer in(s);
            int t=in.get_PN();
            if(t!=msgid::blockZ)
                throw CommonError("if(t!=msgid::blockZ)");
            unpack(in);
        }
        BigInt prev_epoch;
        BLOCK_id prev_root_hash;
        BLOCK_id new_root_hash1;
        THASH_id attachment_hash;
        // THASH_id instruction_reports_hash;
        // THASH_id rewards_hash;
        // THASH_id fee_hash;
        // THASH_id trs_hash;
        std::string payload_heart_bit;
        // std::vector<THASH_id> transaction_hashes;
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            message_base::pack(b);
            b<<prev_epoch;
            b<<prev_root_hash;
            b<<new_root_hash1;
            b<<attachment_hash;
            b<<payload_heart_bit;
            // b<<transaction_hashes;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            message_base::unpack(b);
            b>>prev_epoch;
            b>>prev_root_hash;
            b>>new_root_hash1;
            b>>attachment_hash;
            b>>payload_heart_bit;
            // b>>transaction_hashes;
        }

    };

    struct publish_block: public message_base
    {
        publish_block():message_base(msgid::publish_block)
        {

        }
        publish_block(const std::string& s):message_base(msgid::publish_block)
        {
            inBuffer in(s);
            int t=in.get_PN();
            if(t!=msgid::publish_block)
                throw CommonError("if(t!=msgid::publish_block)");
            unpack(in);
        }
        BigInt epoch;
        attachment_data att_data;
        std::string block_accepted_req;
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            message_base::pack(b);
            b<<epoch;
            b<<att_data;
            b<<block_accepted_req;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            message_base::unpack(b);
            b>>epoch;
            b>>att_data;
            b>>block_accepted_req;
        }

    };
    struct block_accepted_req: public message_base
    {
        block_accepted_req():message_base(msgid::block_accepted_req)
        {

        }
        block_accepted_req(inBuffer &in):message_base(msgid::block_accepted_req)
        {
            unpack(in);
        }
        block_accepted_req(const std::string &s):message_base(msgid::block_accepted_req)
        {
            inBuffer in(s);
            auto t=in.get_PN();
            if(t!=msgid::block_accepted_req)
                throw CommonError("if(t!=msgid::block_accepted_req)");
            unpack(in);
        }
        std::string leader_certificateZ;
        std::string block_payload;
        std::vector<NODE_id> node_validators;
        blst_cpp::AggregateSignature agg_sig;
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            message_base::pack(b);
            b<<leader_certificateZ<<block_payload<<node_validators<<agg_sig;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            message_base::unpack(b);
            b>>leader_certificateZ>>block_payload>>node_validators>>agg_sig;
        }
    };
    struct block_accepted_rsp: public message_base
    {
        block_accepted_rsp():message_base(msgid::block_accepted_rsp)
        {

        }
        block_accepted_rsp(inBuffer &in):message_base(msgid::block_accepted_rsp)
        {
            unpack(in);
        }
        NODE_id node_signer;
        BLOCK_id new_root_hash;
        blst_cpp::Signature sig_bls;
        void sign(const blst_cpp::SecretKey& sk)
        {
            sig_bls.sign(sk, new_root_hash.container);
        }
        bool verify(const blst_cpp::PublicKey & pk)
        {
            return sig_bls.verify(pk,new_root_hash.container);
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            message_base::pack(b);
            b<<new_root_hash<<sig_bls<<node_signer;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            message_base::unpack(b);
            b>>new_root_hash>>sig_bls>>node_signer;
        }
    };


    struct block_response: public message_base
    {
        block_response():message_base(msgid::block_response)
        {

        }
        block_response(inBuffer& in):message_base(msgid::block_response)
        {
            unpack(in);
        }
        std::string payload_block;
        blst_cpp::Signature sig;
        NODE_id node_validator;
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            message_base::pack(b);
            b<<payload_block;
            b<<sig;
            b<<node_validator;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            message_base::unpack(b);
            b>>payload_block;
            b>>sig;
            b>>node_validator;
        }
        void sign(const blst_cpp::SecretKey &sk)
        {
            // Blake2bHasher h;
            // h.update(payload_block);
            sig.sign(sk, blake2b_hash(payload_block).container);
        }
        bool verify(const blst_cpp::PublicKey &pk) const
        {
            return sig.verify(pk, blake2b_hash(payload_block).container);
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


    struct get_blocks_req: public message_base
    {
        get_blocks_req():message_base(msgid::get_blocks_req)
        {

        }
        get_blocks_req(inBuffer &in):message_base(msgid::get_blocks_req)
        {
            unpack(in);
        }
        BigInt myEpoch;
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            message_base::pack(b);
            b<<myEpoch;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            message_base::unpack(b);
            b>>myEpoch;
        }
    };

    struct get_blocks_rsp: public message_base
    {
        get_blocks_rsp():message_base(msgid::get_blocks_rsp)
        {

        }
        get_blocks_rsp(inBuffer &in):message_base(msgid::get_blocks_rsp)
        {
            unpack(in);
        }
        std::vector<std::pair<BigInt,std::string> > blocks_Z;
        BigInt lastEpoch;
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            message_base::pack(b);
            b<<blocks_Z<<lastEpoch;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            message_base::unpack(b);
            b>>blocks_Z>>lastEpoch;
        }
    };


}

