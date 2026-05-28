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
// #include "msg.h"
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


namespace msgid
{
    enum MSG_ID
    {
        user_message_req,
        // user_request,
        // get_user_status_req,
        // get_user_status_rsp,
        HeartBeatREQ,HeartBeatRSP,
        LeaderCertificate, ValidateBlockREQ, ValidateBlockRSP, BlockInfo, BlockAcceptedREQ,BlockAcceptedRSP, GetTransactionREQ,GetTransactionRSP,
        BlockDBStore, GetSavedBlocksREQ,GetSavedBlocksRSP, DoHeartBeatREQ, ConfirmLeaderREQ, ConfirmLeaderRSP,
        // InstructionList,
        TX,
        // TxMint,
        attachment_data,
        GetUserStatusREQ,
        GetUserStatusRSP
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
            MUTEX_INSPECTOR;
            Blake2bHasher h;
            for(auto& z:payload)
                h.update(z);
            h.update(nonce.toString());
            return verify_ed_pk(address_pk_ed,signature,h.final());
        }
        void sign(const std::string &sk)
        {
            MUTEX_INSPECTOR;
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

}

// namespace MsgData
// {


//     struct BlockInfo;

// }

// bool verify_tx(const std::string& msg);

