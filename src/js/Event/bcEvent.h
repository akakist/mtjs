#pragma once
// #include "signedBuffer.h"
#include "THASH_id.h"
#include "IDatabase.h"
#include "blst_cp.h"
#include "NODE_id.h"
#include "tree.h"
#include "msg.h"
#include "md/md_TX.h"
namespace ServiceEnum
{
    const SERVICE_id Node(ghash("@g_Node"));
    const SERVICE_id BlockValidator(ghash("@g_BlockValidator"));
    const SERVICE_id TxValidator(ghash("@g_TxValidator"));
    const SERVICE_id BroadcasterTree(ghash("@g_BroadcasterTree"));
    const SERVICE_id GrainReader(ghash("@g_GrainReader"));
    const SERVICE_id BlockStreamer(ghash("@g_BlockStreamer"));
    // const SERVICE_id LeaderElection(ghash("@g_LeaderElection"));

}
namespace bcEventEnum
{
    const EVENT_id AddTxREQ(ghash("@g_AddTxREQ"));
    const EVENT_id AddTxRSP(ghash("@g_AddTxRSP"));
    const EVENT_id NodeMsgREQ(ghash("@g_NodeMsgREQ"));
    const EVENT_id NodeMsgRSP(ghash("@g_NodeMsgRSP"));
    const EVENT_id ClientMsg(ghash("@g_bcClientMsg"));
    const EVENT_id ClientMsgReply(ghash("@g_bcClientMsgReply"));
    const EVENT_id ClientTxSubscribeREQ(ghash("@g_ClientTxSubscribeREQ"));
    const EVENT_id ClientTxSubscribeRSP(ghash("@g_ClientTxSubscribeRSP"));
    const EVENT_id ServiceInit(ghash("@g_ServiceInit"));
    const EVENT_id InvalidateRoot(ghash("@g_InvalidateRoot"));
    const EVENT_id BroadcastMessage(ghash("@g_BroadcastMessage"));
    const EVENT_id SendToChild(ghash("@g_SendToChild"));
    const EVENT_id SendToChildAck(ghash("@g_SendToChildAck"));
    const EVENT_id StreamBlock(ghash("@g_StreamBlock"));
    const EVENT_id HeartBeatREQ(ghash("@g_HeartBeatREQ"));
    const EVENT_id PutTransactionREQ(ghash("@g_PutTransactionREQ"));
    const EVENT_id GetUserStatusREQ(ghash("@g_GetUserStatusREQ"));
    const EVENT_id GetUserStatusRSP(ghash("@g_GetUserStatusRSP"));

}

namespace bcEvent
{

    class NodeMsgREQ : public Event::Base
    {

    public:
        static Base *construct(const route_t &r)
        {
            return new NodeMsgREQ(r);
        }
        NODE_id node_signer;
        std::string signature;
        std::string msg_payload;

        NodeMsgREQ(const NODE_id &_node_signer, const std::string &sig,
                   const std::string &_msg,
                   const route_t &r)
            : Base(bcEventEnum::NodeMsgREQ, r),
              node_signer(_node_signer), signature(sig), msg_payload(_msg)
        {
        }

        NodeMsgREQ(const route_t &r)
            : Base(bcEventEnum::NodeMsgREQ, r) {}
        void unpack(inBuffer &o)
        {

            o >> node_signer >> signature >> msg_payload;
        }
        void pack(outBuffer &o) const
        {

            o << node_signer << signature << msg_payload;
        }
    };
    class NodeMsgRSP : public Event::Base
    {

    public:
        static Base *construct(const route_t &r)
        {
            return new NodeMsgRSP(r);
        }
        NODE_id node_signer;
        std::string signature;
        std::string msg_payload;

        NodeMsgRSP(const NODE_id &_node_signer, const std::string &sig,
                   const std::string &_msg,
                   const route_t &r)
            : Base(bcEventEnum::NodeMsgRSP, r),
              node_signer(_node_signer), signature(sig), msg_payload(_msg)
        {
        }

        NodeMsgRSP(const route_t &r)
            : Base(bcEventEnum::NodeMsgRSP, r) {}
        void unpack(inBuffer &o)
        {

            o >> node_signer >> signature >> msg_payload;
        }
        void pack(outBuffer &o) const
        {

            o << node_signer << signature << msg_payload;
        }
    };


    class ClientMsg : public Event::Base
    {

    public:
        static Base *construct(const route_t &r)
        {
            return new ClientMsg(r);
        }
        std::string msg;

        ClientMsg(const std::string &_msg, const route_t &r)
            : Base(bcEventEnum::ClientMsg, r),
              msg(_msg)
        {
        }

        ClientMsg(const route_t &r)
            : Base(bcEventEnum::ClientMsg, r) {}
        void unpack(inBuffer &o)
        {
            o >> msg;
        }
        void pack(outBuffer &o) const
        {
            o << msg;
        }
    };
    class ClientMsgReply : public Event::Base
    {

    public:
        static Base *construct(const route_t &r)
        {
            return new ClientMsgReply(r);
        }
        THASH_id hash_of_request;
        std::string msg;

        ClientMsgReply(const THASH_id &_hash, const std::string &_msg, const route_t &r)
            : Base(bcEventEnum::ClientMsgReply, r),
              hash_of_request(_hash), msg(_msg)
        {
        }

        ClientMsgReply(const route_t &r)
            : Base(bcEventEnum::ClientMsgReply, r) {}
        void unpack(inBuffer &o)
        {

            o >> hash_of_request >> msg;
        }
        void pack(outBuffer &o) const
        {

            o << hash_of_request << msg;
        }
    };
    class ClientTxSubscribeREQ : public Event::Base
    {

    public:
        static Base *construct(const route_t &r)
        {
            return new ClientTxSubscribeREQ(r);
        }

        ClientTxSubscribeREQ(const route_t &r)
            : Base(bcEventEnum::ClientTxSubscribeREQ, r) {}
        void unpack(inBuffer &o)
        {

            // o>>msg;
        }
        void pack(outBuffer &o) const
        {

            // o<<msg;
        }
    };

    class ClientTxSubscribeRSP : public Event::Base
    {

    public:
        static Base *construct(const route_t &r)
        {
            return new ClientTxSubscribeRSP(r);
        }
        std::string msg;

        ClientTxSubscribeRSP(const std::string &_msg, const route_t &r)
            : Base(bcEventEnum::ClientTxSubscribeRSP, r),
              msg(_msg)
        {
        }

        ClientTxSubscribeRSP(const route_t &r)
            : Base(bcEventEnum::ClientTxSubscribeRSP, r) {}
        void unpack(inBuffer &o)
        {

            o >> msg;
        }
        void pack(outBuffer &o) const
        {

            o << msg;
        }
    };

    class ServiceInit : public Event::NoPacked
    {

    public:
        static Base *construct(const route_t &r)
        {
            return NULL;
        }
        ServiceInit(blst_cpp::SecretKey my_sk_bls_,
                    std::string my_sk_ed_, const NODE_id &this_node_name_, const REF_getter<IDatabase> &db_,
                    const route_t &r)
            : NoPacked(bcEventEnum::ServiceInit, r), my_sk_bls(my_sk_bls_), my_sk_ed(my_sk_ed_), this_node_name(this_node_name_), db(db_) {}

        blst_cpp::SecretKey my_sk_bls;
        std::string my_sk_ed;

        NODE_id this_node_name;
        REF_getter<IDatabase> db;
    };

    class InvalidateRoot : public Event::NoPacked
    {
    public:
        static Base *construct(const route_t &r)
        {
            return NULL;
        }
        InvalidateRoot(const route_t &r)
            : NoPacked(bcEventEnum::InvalidateRoot, r) {}
    };

    class BroadcastMessage : public Event::NoPacked
    {

    public:
        static Base *construct(const route_t &r)
        {
            return NULL;
        }
        BroadcastMessage(const SERVICE_id &dstService_, const NODE_id& _node_signer,
            const std::string& _signature_pl,
            const std::string &m, const route_t &r)
            : NoPacked(bcEventEnum::BroadcastMessage, r), dstService(dstService_),
            node_signer(_node_signer),signature_pl(_signature_pl),
            msg(m) {}

        SERVICE_id dstService;
        const NODE_id node_signer;
        const std::string signature_pl;
        const std::string msg;
    };
    class SendToChild : public Event::Base
    {

    public:
        static Base *construct(const route_t &r)
        {
            return new SendToChild(r);
        }
        SendToChild(const NODE_id& _node_signer, const std::string& signature_pld, const std::string &_payload, const BroadcasterTree::TreeNode &_bt, const SERVICE_id &_dstSvs, const NODE_id &_dstNodeName, const route_t &r)
            : Base(bcEventEnum::SendToChild, r), node_signer(_node_signer),payload_signature(signature_pld), 
            payload(_payload), bt(_bt), dst_service(_dstSvs), dstNodeName(_dstNodeName) {}

        std::string hash() const
        {
            MUTEX_INSPECTOR;
            Blake2bHasher h;
            h.update(node_signer.container);
            h.update(payload_signature);
            h.update(bt.node.name.container);
            return h.final();
        }
        NODE_id node_signer;
        std::string payload_signature;
        std::string payload;
        BroadcasterTree::TreeNode bt;
        SERVICE_id dst_service;
        NODE_id dstNodeName;

        SendToChild(const route_t &r)
            : Base(bcEventEnum::SendToChild, r) {}

        void unpack(inBuffer &o)
        {

            o >>node_signer>>payload_signature >> payload >> bt >> dst_service >> dstNodeName;
        }
        void pack(outBuffer &o) const
        {

            o<< node_signer<<payload_signature << payload << bt << dst_service << dstNodeName;
        }
    };
    class SendToChildAck : public Event::Base
    {

    public:
        static Base *construct(const route_t &r)
        {
            return new SendToChildAck(r);
        }
        SendToChildAck(const std::string &_hash, const route_t &r)
            : Base(bcEventEnum::SendToChildAck, r), hash(_hash) {}

        std::string hash;

        SendToChildAck(const route_t &r)
            : Base(bcEventEnum::SendToChildAck, r) {}

        void unpack(inBuffer &o)
        {

            o >> hash;
        }
        void pack(outBuffer &o) const
        {

            o << hash;
        }
    };
    class StreamBlock : public Event::NoPacked
    {

    public:
        static Base *construct(const route_t &r)
        {
            return NULL;
        }
        StreamBlock(const std::string &_payload, const route_t &r)
            : NoPacked(bcEventEnum::StreamBlock, r), payload(_payload) {}

        const std::string payload;
    };
    // struct NetworkBase: public Event::Base
    // {
    //     NetworkBase(const EVENT_id& id, const route_t& r):Event::Base(id,r) {}
    //     virtual void  update(Blake2bHasher &h) const =0;
    // };
    class PutTransactionREQ : public Event::NoPacked
    {

    public:
        static Base *construct(const route_t &r)
        {
            return NULL;
        }
        PutTransactionREQ(const REF_getter<MsgData::TX> & _tx, const route_t &r)
            : NoPacked(bcEventEnum::PutTransactionREQ, r), tx(_tx) {}

        const REF_getter<MsgData::TX> tx;
    };
    class AddTxREQ : public Event::Base
    {

    public:
        static Base *construct(const route_t &r)
        {
            return new AddTxREQ(r);
        }
        AddTxREQ(const REF_getter<MsgData::TX> &_tx, const route_t &r)
            : Base(bcEventEnum::AddTxREQ, r), tx(_tx) {}

        REF_getter<MsgData::TX> tx;

        AddTxREQ(const route_t &r)
            : Base(bcEventEnum::AddTxREQ, r) {}


        void unpack(inBuffer &o)
        {
            o>>tx;
        }
        void pack(outBuffer &o) const
        {
            o<<tx;
        }
    };
    class AddTxRSP : public Event::Base
    {

    public:
        static Base *construct(const route_t &r)
        {
            return new AddTxRSP(r);
        }
        AddTxRSP(const THASH_id& h, int ec, const std::string &em, const route_t &r)
            : Base(bcEventEnum::AddTxRSP, r), tx_hash(h),errcode(ec), errmsg(em) {}

        // REF_getter<MsgData::TX> tx;
        THASH_id tx_hash;
        int errcode;
        std::string errmsg;

        AddTxRSP(const route_t &r)
            : Base(bcEventEnum::AddTxRSP, r) {}

        void unpack(inBuffer &o)
        {
            o>>tx_hash>>errcode>>errmsg;
        }
        void pack(outBuffer &o) const
        {
            o<<tx_hash<<errcode<<errmsg;
        }
    };
    class GetUserStatusREQ : public Event::Base
    {

    public:
        static Base *construct(const route_t &r)
        {
            return new GetUserStatusREQ(r);
        }
        GetUserStatusREQ(const std::string& _user_pk_ed, const route_t &r)
            : Base(bcEventEnum::GetUserStatusREQ, r), user_pk_ed(_user_pk_ed) {}

        std::string user_pk_ed;

        GetUserStatusREQ(const route_t &r)
            : Base(bcEventEnum::GetUserStatusREQ, r) {}

        void unpack(inBuffer &o)
        {
            o>>user_pk_ed;
        }
        void pack(outBuffer &o) const
        {
            o<<user_pk_ed;
        }
    };
}