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
#include "md/md_DoHeartBeatREQ.h"
#include "md/md_ConfirmLeaderREQ.h"
#include "md/md_ConfirmLeaderRSP.h"
#include "md/md_LcEnvelopeREQ.h"
#include "md/md_DoYouHaveBlockREQ.h"
#include "md/md_DoYouHaveBlockRSP.h"
#include "md/md_LcREQ.h"
#include "md/md_LcRSP.h"
#include "dbHistory.h"
#include "t_params.h"
#include "cached_state.h"
#include "contract_rt.h"
#define BROADCAST_ACK_TIMEDOUT_SEC 0.2
// #define HEART_BEAT_INTERVAL_SEC 5

enum State
{
    STATE_NORMAL,STATE_SYNCING
};

namespace Node
{
    enum timers
    {
        // TIMER_START_HEART_BEAT,
        TIMER_RESTART_BLOCK,
        TIMER_PERIODIC_CLOCK,
        TIMER_VALIDATE_BLOCK_DELAY,
        TIMER_SYNC_TIMEDOUT,
        TIMER_LC_REQ_TIMEDOUT
    };
    struct heart_beat_node_info
    {
        heart_beat_node_info() : leader_cert_2(nullptr) {

            // responses.clear();
            // clear();

        }
        int64_t TIMER_VALIDATE_BLOCK_DELAY_set=0;
        int64_t request_for_transactions_sent=0;
        int64_t confirm_leader_sent=0;
        REF_getter< MsgData::LeaderCertificate> leader_cert_2;
        std::map<NODE_id,REF_getter<MsgData::HeartBeatRSP> > HeartBeatRSP_m;
        std::map<NODE_id,REF_getter<MsgData::ConfirmLeaderRSP> > ConfirmLeaderRSP_m;
        std::set<NODE_id> transaction_responders;
        uint64_t request_for_transactions_time=0;

        // void clear__1()
        // {
        //     request_for_transactions_sent=false;
        //     leader_cert_2=nullptr;
        //     HeartBeatRSP_m.clear();
        //     transaction_responders.clear();
        // }


    };
    struct heart_beat_info
    {
        heart_beat_node_info leader_info;
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

        void do_heart_beat();

        bool LcEnvelopeREQ(const MsgData::LcEnvelopeREQ* r, const NODE_id & src_node, const route_t& route);
        bool HeartBeatREQ(const MsgData::HeartBeatREQ *h,const MsgData::LeaderCertificate *remote_prev_lc, const NODE_id &src_node, const route_t &route);
        void reply_HeartBeatRSP(const MsgData::HeartBeatREQ *h, const route_t &route);

        bool HeartBeatRSP(const MsgData::HeartBeatRSP* r, const NODE_id & src_node, const route_t& route);;
        bool GetTransactionREQ(const MsgData::GetTransactionREQ* r, const NODE_id & src_node, const route_t& route);
        bool GetTransactionRSP(const MsgData::GetTransactionRSP* r, const NODE_id & src_node, const route_t& route);
        bool ValidateBlockREQ(const MsgData::ValidateBlockREQ* r, const NODE_id & src_node, const route_t& route);
        bool ValidateBlockRSP(const MsgData::ValidateBlockRSP* r, const NODE_id & src_node, const route_t& route);
        bool BlockAcceptedREQ(const MsgData::BlockAcceptedREQ* r, const NODE_id & src_node, const route_t& route);

        bool GetSavedBlocksRSP(const MsgData::GetSavedBlocksRSP* r, const NODE_id & src_node, const route_t& route);
        bool GetSavedBlocksREQ(const MsgData::GetSavedBlocksREQ* r, const NODE_id & src_node, const route_t& route);
        bool ConfirmLeaderREQ(const MsgData::ConfirmLeaderREQ* m, const NODE_id & src_node, const route_t& route);
        bool ConfirmLeaderRSP(const MsgData::ConfirmLeaderRSP* m, const NODE_id & src_node, const route_t& route);

        bool DoYouHaveBlockREQ(const MsgData::DoYouHaveBlockREQ* m, const NODE_id & src_node, const route_t& route);
        bool DoYouHaveBlockRSP(const MsgData::DoYouHaveBlockRSP* m, const NODE_id & src_node, const route_t& route);
        bool LcREQ(const MsgData::LcREQ* m, const NODE_id & src_node, const route_t& route);
        bool LcRSP(const MsgData::LcRSP* m, const NODE_id & src_node, const route_t& route);

        bool NodeMsgREQ(const bcEvent::NodeMsgREQ* m);
        bool NodeMsgRSP(const bcEvent::NodeMsgRSP* m);


        void make_leader_certificate();
        bool isNodeGreaterOrEqual(const NODE_id& nodeLeft, const NODE_id& nodeRight);
        int nodeDistanceToLeader(const NODE_id& node);



        void do_request_for_transactions( Node::heart_beat_node_info& li);

        void broadcast_MsgEvent(const REF_getter<MsgData::Base>& p);
        void pass_NodeMsgRSP(const MsgData::Base *e,const route_t& r);

        void do_InvalidateRoot();


        struct block
        {
            std::map<THASH_id /*blockinfo hash*/,REF_getter<MsgData::BlockInfo> > blockInfo;
            std::map<THASH_id /*blockinfo hash*/, std::vector<REF_getter<MsgData::ValidateBlockRSP> > >responses;
            int64_t block_accepted_sent=0;
            heart_beat_info    heart_beat_store;
        };
        _db_to_save db_to_save_Z;


        std::map<THASH_id, REF_getter<MsgData::TX> >  transaction_pool_of_leader;
        std::map<BLOCK_id,block> blocks_leader;
        

        struct client_block_info
        {
            REF_getter<MsgData::BlockDBStore> blockDBStore=nullptr;
            REF_getter<MsgData::attachment_data> att_data= nullptr;

        };
        std::map<BLOCK_id, client_block_info> c_blocks;
        struct client_leader_info
        {
            NODE_id node_leader;
            int64_t heart_beat_sent=0;
        };

        std::map<BLOCK_id, client_leader_info> cli_leader_info;

        struct _sync
        {
            bool do_you_have_sent=false;
            std::set<NODE_id> havers;
        };
        std::map<BLOCK_id,_sync> syncs;

        BLOCK_id prev_root_hash_Z;
        void do_start_block();

        std::map<EPOCH_id, std::map<NODE_id, REF_getter<MsgData::LeaderCertificate> > >lc_responses;

        void collectTransactions();
        BLOCK_id execute_block(t_params &t,  const REF_getter<MsgData::LeaderCertificate> &lc);

        void do_sync(const NODE_id &src_node);

        // bool CheckState(const MsgData::HeartBeatREQ *r, const NODE_id & src_node);

        void calc_fee_rewards_nodes(t_params& t, const REF_getter<MsgData::LeaderCertificate> &lc);

        BLOCK_id proceed_merkle_on_transaction_pool_hashers(const REF_getter<root_data> &r);
    
        bool verify_leader_certificate(const REF_getter<MsgData::LeaderCertificate>& lc);

        void execute_transaction(const THASH_id &tx_id, t_params &t, const ADDRESS_id &senderAddress,
                         const std::string &tx_cmds, const REF_getter<fee_calcer> &by, const EPOCH_id& epoch);


        REF_getter<root_data> root=nullptr;
        REF_getter<IDatabase> db_state=nullptr;
        REF_getter<DB_history> db_history=nullptr;


        uint64_t last_activity_time=0;
        std::string sqlite_pn;
        std::string rocksdb_path;
        std::set<msockaddr_in> rpc_addr;
        void logNode(const char* fmt, ...);
        IInstance *iInstance=NULL;
        State state_Z=STATE_NORMAL;
        bool stage_is_working = false;

        std::vector<std::string> telnet_data_path;

        blst_cpp::SecretKey my_sk_bls;
        std::string my_sk_ed;

        NODE_id this_node_name;

        std::set<msockaddr_in> web_addr;

        std::string my_sk_bls_env_key;
        std::string my_sk_ed_env_key;

        MsgFactory msgFactory;

        std::map<CONTRACT_id, REF_getter<contract_rt> > contracts;
        std::optional<std::string> load_contract(const CONTRACT_id& contract);
        std::optional<std::string> execute_contract(const CONTRACT_id& ct, const std::string & method, const yyjson::Value& params);

        JSRuntime *contract_runtime=NULL;


    };

}

