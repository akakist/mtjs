#pragma once

#include "broadcaster.h"

// #include "signedBuffer.h"

#include "listenerBuffered1Thread.h"
#include <map>
#include <rocksdb/db.h>
#include "Events/System/Run/startServiceEvent.h"
#include "Events/Tools/telnetEvent.h"
#include "Events/Tools/webHandlerEvent.h"
#include "Events/System/Net/httpEvent.h"
#include "Events/System/timerEvent.h"
#include "Events/System/Net/httpEvent.h"
#include "Event/bcEvent.h"
#include "root_contract.h"
#include "msg.h"
#include "tr_exec.h"
#include "bigint.h"
#include "TRANSACTION_id.h"
#include "THASH_id.h"
#include "NODE_id.h"
#include "db_to_save.h"
#include "blst_cp.h"

#define BROADCAST_ACK_TIMEDOUT_SEC 0.2
// #define HEART_BEAT_TIMEDOUT_SEC 5
#define HEART_BEAT_INTERVAL_SEC 5

enum State
{
    NORMAL,SYNCING
};

namespace Node
{
    enum timers
    {
        TIMER_START_HEART_BEAT,
        TIMER_RESTART_BLOCK,
    };
    // struct heart_beat_responce2
    // {
    //     BigInt stake_34;
    //     REF_getter<MsgEvt::HeartBeatRSP> rsp;
    //     heart_beat_responce2():rsp(nullptr)
    //     {
    //         stake_34=0;
    //     }
    // };
    struct heart_beat_node_info
    {
        heart_beat_node_info() : leader_cert_2(nullptr) {

            // responses.clear();
            clear();

        }
        bool request_for_transactions_sent=false;
        bool confirm_leader_sent=false;
        REF_getter< MsgEvt::LeaderCertificate> leader_cert_2;
        std::map<NODE_id,REF_getter<MsgEvt::HeartBeatRSP> > HeartBeatRSP_m;
        std::map<NODE_id,REF_getter<MsgEvt::ConfirmLeaderRSP> > ConfirmLeaderRSP_m;
        std::set<NODE_id> transaction_responders;

        void clear()
        {
            request_for_transactions_sent=false;
            leader_cert_2=nullptr;
            HeartBeatRSP_m.clear();
            transaction_responders.clear();
        }


    };
    struct heart_beat_info
    {
        // NODE_id node_leader;
        heart_beat_node_info leader_info;
        heart_beat_info()
        {

        }
        void clear()
        {
            // node_leader.container.clear();
            // leader_info.clear();
        }
    };
    class Service:
        public UnknownBase,
        public ListenerBuffered1Thread,
        public Broadcaster
    {
        bool on_startService(const systemEvent::startService*);
        bool on_timer(const timerEvent::TickTimer*);
        bool on_alarm(const timerEvent::TickAlarm*);
        bool handleEvent(const REF_getter<Event::Base>& e);




        Service(const SERVICE_id&, const std::string&  nm, IInstance *ins);
        ~Service();


        bool on_CommandEntered(const telnetEvent::CommandEntered*);

    public:
        void deinit()
        {
            ListenerBuffered1Thread::deinit();
        }

        static UnknownBase* construct(const SERVICE_id& id, const std::string&  nm,IInstance* obj)
        {
            XTRY;
            return new Service(id,nm,obj);
            XPASS;
        }
        bool on_RequestIncoming(const webHandlerEvent::RequestIncoming* e);


        bool RequestIncoming(const httpEvent::RequestIncoming* e);
        bool PutTransactionREQ(const bcEvent::PutTransactionREQ* e);
        // bool ClientTxSubscribeREQ(const bcEvent::ClientTxSubscribeREQ*);
        bool Msg(const bcEvent::Msg*, bool fromNetwork);
        bool MsgReply(const bcEvent::MsgReply*, bool fromNetwork);

        void resetTimer();


        void do_heart_beat();

        // bool HeartBeatREQ(const MsgEvt::HeartBeatREQ* h,const std::string &heart_beat_payload, const route_t& route);
        bool HeartBeatREQ(const MsgEvt::HeartBeatREQ* h,const NODE_id &src_node, const route_t& route);
        bool HeartBeatRSP(const MsgEvt::HeartBeatRSP* r, const NODE_id & src_node, const route_t& route);;
        bool GetTransactionREQ(const MsgEvt::GetTransactionREQ* r, const NODE_id & src_node, const route_t& route);
        bool GetTransactionRSP(const MsgEvt::GetTransactionRSP* r, const NODE_id & src_node, const route_t& route);
        bool ValidateBlockREQ(const MsgEvt::ValidateBlockREQ* r, const NODE_id & src_node, const route_t& route);
        bool ValidateBlockRSP(const MsgEvt::ValidateBlockRSP* r, const NODE_id & src_node, const route_t& route);
        bool BlockAcceptedREQ(const MsgEvt::BlockAcceptedREQ* r, const NODE_id & src_node, const route_t& route);
        bool BlockAcceptedRSP(const MsgEvt::BlockAcceptedRSP* r, const NODE_id & src_node, const route_t& route);

        bool GetSavedBlocksRSP(const MsgEvt::GetSavedBlocksRSP* r, const NODE_id & src_node, const route_t& route);
        bool GetSavedBlocksREQ(const MsgEvt::GetSavedBlocksREQ* r, const NODE_id & src_node, const route_t& route);
        bool DoHeartBeatREQ(const MsgEvt::DoHeartBeatREQ* r, const NODE_id & src_node, const route_t& route);
        bool ConfirmLeaderREQ(const MsgEvt::ConfirmLeaderREQ* m, const NODE_id & src_node, const route_t& route);
        bool ConfirmLeaderRSP(const MsgEvt::ConfirmLeaderRSP* m, const NODE_id & src_node, const route_t& route);

        void initDB();

        // void on_heart_beat_rsp(const msg::heart_beat_rsp& hbr);

        void make_leader_certificate();
        bool isNodeGreaterOrEqual(const NODE_id& nodeLeft, const NODE_id& nodeRight);
        int nodeDistanceToLeader(const NODE_id& node);



        void do_request_for_transactions(const Node::heart_beat_node_info& li);



        struct Round
        {

        };

        struct block
        {
            REF_getter<MsgEvt::BlockInfo> block_payload=nullptr;
            std::vector<REF_getter<MsgEvt::ValidateBlockRSP> > responses;
            // BigInt stake_validators;
            // std::map<NODE_id /*validator*/, blst_cpp::Signature> sigs;



            std::vector<std::pair<NODE_id/*node*/,blst_cpp::Signature> > signs;
            bool executed=false;
            bool block_accepted_sent=false;
            bool heart_bit_sent_on_block_accepted_rsp=false;

            std::map<NODE_id, REF_getter<MsgEvt::BlockAcceptedRSP> > acceptors;

            // int round_=0;
            heart_beat_info    heart_beat_store;

        };
        _db_to_save db_to_save_Z;
        REF_getter<MsgEvt::BlockDBStore> prepareBlockDBStore(const t_params& t);

        // time_t last_access_time_hbZ=0; // heart-_bit last tick time
        REF_getter<MsgEvt::LeaderCertificate> last_leader_cert=nullptr;

        std::map<THASH_id, TRANSACTION_body>  transaction_pool_of_leader;
        std::map<BLOCK_id,block> blocks_leader;
        NODE_id node_leader_for_client;

        // struct _prepared_block
        // {
        //     attachment_data att_data;
        //     BigInt epoch;
        //     void clear()
        //     {
        //         att_data.clear();
        //         epoch=0;
        //     }
        // };
        // _prepared_block prepared_block;
        REF_getter<MsgEvt::BlockDBStore> blockDBStore=nullptr;
        // void do_client_tx_report(const msg::publish_block &pb);
//
        // void setBlockId(const BLOCK_id& b)
        // {
        //     prev_block_hash_Z=b;
        //     auto err=db->put_cell("#root_hash#",b.container);
        // }
        BLOCK_id prev_block_hash_Z;
        void do_start_block();

        void collectTransactions();
        void execute_block(t_params &t,const REF_getter<root_data> &rt, const BLOCK_id & bl,  const std::vector<NODE_id> &nodes_in_leader_cert);

        void do_sync(const NODE_id &src_node);

        bool CheckState(const MsgEvt::HeartBeatREQ *r, const NODE_id & src_node);

        void calc_fee_and_rewards(t_params& t, const std::vector<NODE_id> &nodes_in_leader_cert);

        BLOCK_id proceed_merkle_on_transaction_pool_hashers(const REF_getter<root_data> &r);

        REF_getter<root_data> root=nullptr;
        REF_getter<IDatabase> db=nullptr;



        std::string sqlite_pn;
        std::string rocksdb_path;
        std::set<msockaddr_in> rpc_addr;
        void logNode(const char* fmt, ...);
        IInstance *iInstance=NULL;
        State state_Z=NORMAL;

        std::vector<std::string> telnet_data_path;

        blst_cpp::SecretKey my_sk_bls;
        std::string my_sk_ed;

        NODE_id this_node_name;

        std::set<msockaddr_in> web_addr;

        std::string my_sk_bls_env_key;
        std::string my_sk_ed_env_key;

        MsgFactory msgFactory;



    };

}

