#pragma once
// #include "signedBuffer.h"
#include "THASH_id.h"
#include "IDatabase.h"
#include "blst_cp.h"
#include "NODE_id.h"
#include "tree.h"
namespace ServiceEnum
{
    const SERVICE_id Node(ghash("@g_Node"));
    const SERVICE_id BlockValidator(ghash("@g_BlockValidator"));
    const SERVICE_id TxValidator(ghash("@g_TxValidator"));
    const SERVICE_id BroadcasterTree(ghash("@g_BroadcasterTree"));
    const SERVICE_id GrainReader(ghash("@g_GrainReader"));
    const SERVICE_id BlockStreamer(ghash("@g_BlockStreamer"));
    const SERVICE_id LeaderElection(ghash("@g_LeaderElection"));
    
    
}
namespace bcEventEnum
{
    const EVENT_id Msg(ghash("@g_bcMsg"));
    const EVENT_id Msg2(ghash("@g_bcMsg2"));
    const EVENT_id MsgReply(ghash("@g_bcMsgReply"));
    const EVENT_id MsgReply2(ghash("@g_bcMsgReply2"));
    const EVENT_id ClientMsg(ghash("@g_bcClientMsg"));
    const EVENT_id ClientMsgReply(ghash("@g_bcClientMsgReply"));
    const EVENT_id ClientTxSubscribeREQ(ghash("@g_ClientTxSubscribeREQ"));
    const EVENT_id ClientTxSubscribeRSP(ghash("@g_ClientTxSubscribeRSP"));
    const EVENT_id TxValidatorStart(ghash("@g_TxValidatorStart"));
    const EVENT_id TxValidatorStop(ghash("@g_TxValidatorStop"));
    const EVENT_id AddTx(ghash("@g_AddTx"));
    const EVENT_id ServiceInit(ghash("@g_ServiceInit"));
    const EVENT_id GetTransactions(ghash("@g_GetTransactions"));
    const EVENT_id InvalidateRoot(ghash("@g_InvalidateRoot"));
    const EVENT_id BroadcastMessage(ghash("@g_BroadcastMessage"));
    const EVENT_id SendToChild(ghash("@g_SendToChild"));
    const EVENT_id SendToChildAck(ghash("@g_SendToChildAck"));
    const EVENT_id StreamBlock(ghash("@g_StreamBlock"));
    const EVENT_id HeartBeatREQ(ghash("@g_HeartBeatREQ"));
    
    
}

namespace bcEvent 
{

    class Msg: public Event::Base
    {

    public:
        static Base* construct(const route_t &r)
        {
            return new Msg(r);
        }
        std::string msg;


        Msg(const std::string& _msg,   const route_t &r)
            :Base(bcEventEnum::Msg,r),
             msg(_msg)
        {}


        Msg(const route_t& r)
            :Base(bcEventEnum::Msg,r) {}
        void unpack(inBuffer& o)
        {

            o>>msg;
        }
        void pack(outBuffer& o) const
        {

            o<<msg;
        }

    };
    class Msg2: public Event::Base
    {

    public:
        static Base* construct(const route_t &r)
        {
            return new Msg2(r);
        }
        std::string msg;


        Msg2(const std::string& _msg,   const route_t &r)
            :Base(bcEventEnum::Msg2,r),
             msg(_msg)
        {}


        Msg2(const route_t& r)
            :Base(bcEventEnum::Msg2 ,r) {}
        void unpack(inBuffer& o)
        {

            o>>msg;
        }
        void pack(outBuffer& o) const
        {

            o<<msg;
        }

    };
    class MsgReply: public Event::Base
    {

    public:
        static Base* construct(const route_t &r)
        {
            return new MsgReply(r);
        }
        std::string msg;


        MsgReply(const std::string& _msg,   const route_t &r)
            :Base(bcEventEnum::MsgReply,r),
             msg(_msg)
        {}


        MsgReply(const route_t& r)
            :Base(bcEventEnum::MsgReply,r) {}
        void unpack(inBuffer& o)
        {

            o>>msg;
        }
        void pack(outBuffer& o) const
        {

            o<<msg;
        }

    };
    class MsgReply2: public Event::Base
    {

    public:
        static Base* construct(const route_t &r)
        {
            return new MsgReply2(r);
        }
        std::string msg;


        MsgReply2(const std::string& _msg,   const route_t &r)
            :Base(bcEventEnum::MsgReply2,r),
             msg(_msg)
        {}


        MsgReply2(const route_t& r)
            :Base(bcEventEnum::MsgReply2,r) {}
        void unpack(inBuffer& o)
        {

            o>>msg;
        }
        void pack(outBuffer& o) const
        {

            o<<msg;
        }

    };

    class ClientMsg: public Event::Base
    {

    public:
        static Base* construct(const route_t &r)
        {
            return new ClientMsg(r);
        }
        std::string msg;


        ClientMsg(const std::string& _msg,   const route_t &r)
            :Base(bcEventEnum::ClientMsg,r),
            msg(_msg)
        {}


        ClientMsg(const route_t& r)
            :Base(bcEventEnum::ClientMsg,r) {}
        void unpack(inBuffer& o)
        {
            o>>msg;
        }
        void pack(outBuffer& o) const
        {
            o<<msg;
        }

    };
    class ClientMsgReply: public Event::Base
    {

    public:
        static Base* construct(const route_t &r)
        {
            return new ClientMsgReply(r);
        }
        THASH_id hash_of_request;
        std::string msg;


        ClientMsgReply(const THASH_id& _hash, const std::string& _msg, const route_t &r)
            :Base(bcEventEnum::ClientMsgReply,r),
             hash_of_request(_hash), msg(_msg)
        {}


        ClientMsgReply(const route_t& r)
            :Base(bcEventEnum::ClientMsgReply,r) {}
        void unpack(inBuffer& o)
        {

            o>>hash_of_request>>msg;

        }
        void pack(outBuffer& o) const
        {

            o<<hash_of_request<<msg;
        }

    };
    class ClientTxSubscribeREQ: public Event::Base
    {

    public:
        static Base* construct(const route_t &r)
        {
            return new ClientTxSubscribeREQ(r);
        }


        ClientTxSubscribeREQ(const route_t& r)
            :Base(bcEventEnum::ClientTxSubscribeREQ,r) {}
        void unpack(inBuffer& o)
        {

            // o>>msg;
        }
        void pack(outBuffer& o) const
        {

            // o<<msg;
        }

    };
    
class ClientTxSubscribeRSP: public Event::Base
    {

    public:
        static Base* construct(const route_t &r)
        {
            return new ClientTxSubscribeRSP(r);
        }
        std::string msg;


        ClientTxSubscribeRSP(const std::string& _msg,   const route_t &r)
            :Base(bcEventEnum::ClientTxSubscribeRSP,r),
             msg(_msg)
        {}


        ClientTxSubscribeRSP(const route_t& r)
            :Base(bcEventEnum::ClientTxSubscribeRSP,r) {}
        void unpack(inBuffer& o)
        {

            o>>msg;
        }
        void pack(outBuffer& o) const
        {

            o<<msg;
        }

    };
 
class TxValidatorStart: public Event::NoPacked
    {

    public:
        static Base* construct(const route_t &r)
        {
            return NULL;
        }
        TxValidatorStart(const REF_getter<IDatabase> &_db, const route_t& r)
            :NoPacked(bcEventEnum::TxValidatorStart,r),db(_db) {}

         REF_getter<IDatabase> db;

    };
    class TxValidatorStop: public Event::NoPacked
    {

    public:
        static Base* construct(const route_t &r)
        {
            return NULL;
        }
        TxValidatorStop(const route_t& r)
            :NoPacked(bcEventEnum::TxValidatorStop,r) {}

    };
    class AddTx: public Event::NoPacked
    {

    public:
        static Base* construct(const route_t &r)
        {
            return NULL;
        }
        AddTx(const REF_getter<ClientMsg>&m, const route_t& r)
            :NoPacked(bcEventEnum::AddTx,r), msg(m) {}
        
        REF_getter<ClientMsg>msg;

    };
    class ServiceInit: public Event::NoPacked
    {

    public:
        static Base* construct(const route_t &r)
        {
            return NULL;
        }
        ServiceInit(blst_cpp::SecretKey my_sk_bls_,
            std::string my_sk_ed_, const NODE_id& this_node_name_, const REF_getter<IDatabase> &db_,
             const route_t& r)
            :NoPacked(bcEventEnum::ServiceInit,r), my_sk_bls(my_sk_bls_), my_sk_ed(my_sk_ed_),this_node_name(this_node_name_), db(db_) {}
        
        blst_cpp::SecretKey my_sk_bls;
        std::string my_sk_ed;

        NODE_id this_node_name;
        REF_getter<IDatabase> db;



    };

    class GetTransactions: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &r)
        {
            return NULL;
        }
        GetTransactions(const route_t& r)
            :NoPacked(bcEventEnum::GetTransactions,r){}
    };
    class InvalidateRoot: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &r)
        {
            return NULL;
        }
        InvalidateRoot(const route_t& r)
            :NoPacked(bcEventEnum::InvalidateRoot,r){}
    };

    class BroadcastMessage: public Event::NoPacked
    {

    public:
        static Base* construct(const route_t &r)
        {
            return NULL;
        }
        BroadcastMessage(const SERVICE_id& dstService_, const std::string& m, const route_t& r)
            :NoPacked(bcEventEnum::BroadcastMessage,r), dstService(dstService_),  msg(m) {}
        
        SERVICE_id dstService;
        const std::string msg;

    };
    class SendToChild: public Event::Base
    {

    public:
        static Base* construct(const route_t &r)
        {
            return new SendToChild(r);
        }
        SendToChild(const std::string& _payload, const BroadcasterTree::TreeNode &_bt, const SERVICE_id& _dstSvs, const NODE_id& _dstNodeName, const route_t& r)
            :Base(bcEventEnum::SendToChild,r), payload(_payload),bt(_bt),dst_service(_dstSvs),dstNodeName(_dstNodeName) {}
        
        std::string hash() const
        {
            Blake2bHasher h;
            h.update(payload);
            h.update(bt.node.name.container);
            return h.final();
        }    
        std::string payload;
        BroadcasterTree::TreeNode bt;
        SERVICE_id dst_service;
        NODE_id dstNodeName;

        SendToChild(const route_t& r)
            :Base(bcEventEnum::SendToChild,r) {}

        void unpack(inBuffer& o)
        {   

            o>>payload>>bt>>dst_service>>dstNodeName;
        }
        void pack(outBuffer& o) const
        {

            o<<payload<<bt<<dst_service<<dstNodeName;
        }

    };
   class SendToChildAck: public Event::Base
    {

    public:
        static Base* construct(const route_t &r)
        {
            return new SendToChildAck(r);
        }
        SendToChildAck(const std::string& _hash, const route_t& r)
            :Base(bcEventEnum::SendToChildAck,r), hash(_hash) {}
        
        std::string hash;

        SendToChildAck(const route_t& r)
            :Base(bcEventEnum::SendToChildAck,r) {}

        void unpack(inBuffer& o)
        {   

            o>>hash;
        }
        void pack(outBuffer& o) const
        {

            o<<hash;
        }

    };
    class StreamBlock: public Event::NoPacked
    {

    public:
        static Base* construct(const route_t &r)
        {
            return NULL;
        }
        StreamBlock(const std::string& _payload, const route_t& r)
            :NoPacked(bcEventEnum::StreamBlock,r),  payload(_payload) {}
        
        const std::string payload;

    };
    struct NetworkBase: public Event::Base
    {
        NetworkBase(const EVENT_id& id, const route_t& r):Event::Base(id,r) {}
        virtual void  hash(Blake2bHasher &h) const =0;
    };
}