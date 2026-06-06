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
#include "md/md_ConfirmLeaderRSP.h"
#include "md/md_LeaderCertificate.h"
#include "md/md_HeartBeatRSP.h"
#include "md/md_BlockAcceptedREQ.h"
#include "md/md_ValidateBlockRSP.h"
#include "md/md_GetSavedBlocksRSP.h"
#include "md/md_GetSavedBlocksREQ.h"
#include "md/md_ValidateBlockREQ.h"
#include "md/md_GetTransactionRSP.h"
#include "md/md_GetTransactionREQ.h"
#include "md/md_BlockAcceptedREQ.h"
#include "md/md_BlockAcceptedRSP.h"
#include "md/md_DoHeartBeatREQ.h"
#include "md/md_ConfirmLeaderREQ.h"
#include "md/md_ConfirmLeaderRSP.h"
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
        TIMER_PERIODIC_CLOCK,
    };
    struct heart_beat_node_info
    {
        heart_beat_node_info() : leader_cert_2(nullptr) {

            // responses.clear();
            clear();

        }
        bool request_for_transactions_sent=false;
        bool confirm_leader_sent=false;
        REF_getter< MsgData::LeaderCertificate> leader_cert_2;
        std::map<NODE_id,REF_getter<MsgData::HeartBeatRSP> > HeartBeatRSP_m;
        std::map<NODE_id,REF_getter<MsgData::ConfirmLeaderRSP> > ConfirmLeaderRSP_m;
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

        void resetTimer();


        void do_heart_beat();

        // bool HeartBeatREQ(const MsgData::HeartBeatREQ* h,const std::string &heart_beat_payload, const route_t& route);
        bool HeartBeatREQ(const MsgData::HeartBeatREQ* h,const NODE_id &src_node, const route_t& route);
        bool HeartBeatRSP(const MsgData::HeartBeatRSP* r, const NODE_id & src_node, const route_t& route);;
        bool GetTransactionREQ(const MsgData::GetTransactionREQ* r, const NODE_id & src_node, const route_t& route);
        bool GetTransactionRSP(const MsgData::GetTransactionRSP* r, const NODE_id & src_node, const route_t& route);
        bool ValidateBlockREQ(const MsgData::ValidateBlockREQ* r, const NODE_id & src_node, const route_t& route);
        bool ValidateBlockRSP(const MsgData::ValidateBlockRSP* r, const NODE_id & src_node, const route_t& route);
        bool BlockAcceptedREQ(const MsgData::BlockAcceptedREQ* r, const NODE_id & src_node, const route_t& route);
        bool BlockAcceptedRSP(const MsgData::BlockAcceptedRSP* r, const NODE_id & src_node, const route_t& route);

        bool GetSavedBlocksRSP(const MsgData::GetSavedBlocksRSP* r, const NODE_id & src_node, const route_t& route);
        bool GetSavedBlocksREQ(const MsgData::GetSavedBlocksREQ* r, const NODE_id & src_node, const route_t& route);
        bool DoHeartBeatREQ(const MsgData::DoHeartBeatREQ* r, const NODE_id & src_node, const route_t& route);
        bool ConfirmLeaderREQ(const MsgData::ConfirmLeaderREQ* m, const NODE_id & src_node, const route_t& route);
        bool ConfirmLeaderRSP(const MsgData::ConfirmLeaderRSP* m, const NODE_id & src_node, const route_t& route);

        bool NodeMsgREQ(const bcEvent::NodeMsgREQ* m);
        bool NodeMsgRSP(const bcEvent::NodeMsgRSP* m);


        void make_leader_certificate();
        bool isNodeGreaterOrEqual(const NODE_id& nodeLeft, const NODE_id& nodeRight);
        int nodeDistanceToLeader(const NODE_id& node);



        void do_request_for_transactions(const Node::heart_beat_node_info& li);

        void broadcast_MsgEvent(const REF_getter<MsgData::Base>& p);
        void pass_NodeMsgRSP(const MsgData::Base *e,const route_t& r);


        struct Round
        {

        };

        struct block
        {
            REF_getter<MsgData::BlockInfo> blockInfo=nullptr;
            std::vector<REF_getter<MsgData::ValidateBlockRSP> > responses;



            std::vector<std::pair<NODE_id/*node*/,blst_cpp::Signature> > signs;
            bool executed=false;
            bool block_accepted_sent=false;
            bool heart_bit_sent_on_block_accepted_rsp=false;

            std::map<NODE_id, REF_getter<MsgData::BlockAcceptedRSP> > acceptors;

            heart_beat_info    heart_beat_store;

        };
        _db_to_save db_to_save_Z;
        REF_getter<MsgData::BlockDBStore> prepareBlockDBStore(const t_params& t);


        std::map<THASH_id, REF_getter<MsgData::TX> >  transaction_pool_of_leader;
        std::map<BLOCK_id,block> blocks_leader;
        NODE_id node_leader_for_client;

        struct cli_bl
        {
            REF_getter<MsgData::BlockDBStore> blockDBStore=nullptr;
            REF_getter<MsgData::attachment_data> att_data= nullptr;

        };
        std::map<BLOCK_id, cli_bl> c_blocks;
        BLOCK_id prev_root_hash_Z;
        void do_start_block();

        void collectTransactions();
        BLOCK_id execute_block(t_params &t,  const std::vector<NODE_id> &nodes_in_leader_cert);

        void do_sync(const NODE_id &src_node);

        bool CheckState(const MsgData::HeartBeatREQ *r, const NODE_id & src_node);

        void calc_fee_rewards_nodes(t_params& t, const std::vector<NODE_id> &nodes_in_leader_cert);

        BLOCK_id proceed_merkle_on_transaction_pool_hashers(const REF_getter<root_data> &r);

        REF_getter<root_data> root=nullptr;
        REF_getter<IDatabase> db_state=nullptr;
        REF_getter<IDatabase> db_history=nullptr;


        uint64_t last_activity_time=0;
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

