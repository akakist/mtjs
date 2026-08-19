#pragma once

#include "broadcaster.h"

// #include "signedBuffer.h"

#include "listenerBuffered1Thread.h"
#include <map>
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
#include "md/md_DelayNotificationREQ.h"
#include "dbHistory.h"
#include "t_params.h"
#include "cached_state.h"
#include "contract_rt.h"
#include "DBH.h"
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
        TIMER_REPORT_MEM
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
        REF_getter< MsgData::HeartBeatREQ> leader_cert_2;
        std::map<NODE_id,REF_getter<MsgData::HeartBeatRSP> > HeartBeatRSP_m;
        std::map<NODE_id,REF_getter<MsgData::ConfirmLeaderRSP> > ConfirmLeaderRSP_m;
        std::set<NODE_id> transaction_responders;
        uint64_t request_for_transactions_time=0;
        size_t size()
        {
            size_t sz=0;
            sz+=sizeof(TIMER_VALIDATE_BLOCK_DELAY_set);
            sz+=sizeof(request_for_transactions_sent);
            sz+=sizeof(confirm_leader_sent);
            sz+=sizeof(request_for_transactions_time);
            if(leader_cert_2.valid())
                sz+=leader_cert_2->size();
            for(auto &z: HeartBeatRSP_m)
            {
                sz+=z.first.container.size();
                sz+=z.second->size();
            }
            for(auto &z: ConfirmLeaderRSP_m)
            {
                sz+=z.first.container.size();
                sz+=z.second->size();
            }
            for(auto &z: transaction_responders)
            {
                sz+=z.container.size();
            }
            
            return sz;
        }
        void dump(nlohmann::json& j)
        {
            j["HeartBeatRSP_m_SZ"]=HeartBeatRSP_m.size();
            j["ConfirmLeaderRSP_m_SZ"]=ConfirmLeaderRSP_m.size();
            j["transaction_responders_SZ"]=transaction_responders.size();
        }
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
    };
    class Service:
        public UnknownBase,
        public ListenerBuffered1Thread,
        public Broadcaster,
        public DBH_feature
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

        REF_getter<MsgData::HeartBeatREQ> do_heart_beat();

        bool LcEnvelopeREQ(const MsgData::LcEnvelopeREQ* r, const NODE_id & src_node, const route_t& route);
        bool HeartBeatREQ(const MsgData::HeartBeatREQ *h,const MsgData::BlockAcceptedREQ *remote_prev_lc, const NODE_id &src_node, const route_t &route);
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
        // bool LcREQ(const MsgData::LcREQ* m, const NODE_id & src_node, const route_t& route);
        // bool LcRSP(const MsgData::LcRSP* m, const NODE_id & src_node, const route_t& route);

        bool DelayNotificationREQ(const MsgData::DelayNotificationREQ* m, const NODE_id & src_node, const route_t& route);

        bool NodeMsgREQ(const bcEvent::NodeMsgREQ* m);
        bool NodeMsgRSP(const bcEvent::NodeMsgRSP* m);


        // void make_leader_certificate();
        bool isNodeGreaterOrEqual(const NODE_id& nodeLeft, const NODE_id& nodeRight);
        int nodeDistanceToLeader(const NODE_id& node);



        void do_request_for_transactions( Node::heart_beat_node_info& li);

        void broadcast_MsgEvent(const REF_getter<MsgData::Base>& p);
        void pass_NodeMsgRSP(const MsgData::Base *e,const route_t& r);

        void do_InvalidateRoot();


        struct block_leader
        {
            std::map<THASH_id /*blockinfo hash*/,REF_getter<MsgData::BlockInfo> > blockInfo;
            std::map<THASH_id /*blockinfo hash*/, std::vector<REF_getter<MsgData::ValidateBlockRSP> > >responses;
            int64_t block_accepted_sent=0;
            // heart_beat_info    heart_beat_store;
            heart_beat_node_info leader_info;
            size_t size()
            {
                size_t sz=0;
                for(auto& z: blockInfo)
                {
                    sz+=z.first.container.size();
                    sz+=z.second->size();
                }
                for(auto& z: responses)
                {
                    sz+=z.first.container.size();
                    for(auto& x: z.second)
                    {
                        sz+=x->size();

                    }
                }
                sz+=sizeof(block_accepted_sent);
                sz+=leader_info.size();
                return sz;
            }
            void dump(nlohmann::json &j)
            {
                j["blockInfo_SZ"]=blockInfo.size();
                j["responses"]=responses.size();
                // for(auto& z:blockInfo)
                // {
                //     z.second->dump(j["blockInfo"][base16::encode(z.first.container)]);
                // }
                // int idx=0;
                // for(auto& z:responses)
                // {
                //     for(auto& x: z.second)
                //     {
                //         x->dump(j["responses"][base16::encode(z.first.container)][std::to_string(idx++)]);
                //     }
                //     // nlohmann::json jj;

                // }
                // j["block_accepted_sent"]=block_accepted_sent;
                // leader_info.

            }

        };
        _db_to_save db_to_save_Z;


        

        struct block_client
        {
            REF_getter<MsgData::BlockDBStore> blockDBStore=nullptr;
            REF_getter<MsgData::attachment_data> att_data= nullptr;

            int64_t block_validated=0;
            size_t size()
            {
                size_t sz=0;
                if(blockDBStore.valid())
                    sz+=blockDBStore->size();
                if(att_data.valid())
                    sz+=att_data->size();
                sz+=sizeof(block_validated);

                return sz;

            }
        };
        struct client_leader_info
        {
            REF_getter<MsgData::HeartBeatREQ> node_leader;
            // NODE_id node_leader;
            int64_t heart_beat_sent=0;
            int64_t confirm_leader_sent=0;
            size_t size()
            {
                size_t sz=0;
                if(node_leader.valid())
                    sz+=node_leader->size();
                sz+=sizeof(heart_beat_sent);
                sz+=sizeof(confirm_leader_sent);
                return sz;
            }
        };

        struct _sync
        {
            bool do_you_have_sent=false;
            std::set<NODE_id> havers;
            size_t size()
            {
                size_t sz=0;
                sz+=sizeof(do_you_have_sent);
                for(auto &z: havers)
                {
                    sz+=z.container.size();
                }
                return sz;
            }
            void dump(nlohmann::json &j)
            {
                j["havers SZ"]=havers.size();
            }
        };
        std::map<BLOCK_id, block_client> c_blocks;
        std::map<BLOCK_id,block_leader> l_blocks;
        std::map<BLOCK_id, client_leader_info> cli_leader_info;
        std::map<BLOCK_id,_sync> syncs;
        std::map<NODE_id,std::map<int64_t,std::set<int64_t> > > filter_NodeMsgREQ;
        std::map<CONTRACT_id, REF_getter<contract_rt> > contracts;
        std::map<THASH_id, REF_getter<MsgData::TX> >  transaction_pool_of_leader;

        BLOCK_id prev_root_hash_Z()
        {
            if(prev_block.valid())
            return prev_block->blockInfo->new_root_hash1;
            BLOCK_id r;
            r.container="";
            return r;
        }
        uint64_t epoch_current()
        {
            if(prev_block.valid())
            return prev_block->blockInfo->heart_beat->new_epoch+1;
            return 0;

        }
        REF_getter<MsgData::BlockAcceptedREQ> prev_block;
        State state_Z=STATE_NORMAL;
        int64_t stage_is_working = 0;
        uint64_t last_activity_time=0;
        int64_t node_start_timestamp=0;
        int64_t seqId2=0;
        void clear()
        {
            c_blocks.clear();
            l_blocks.clear();
            cli_leader_info.clear();
            // lc_responses.clear();
            syncs.clear();
            transaction_pool_of_leader.clear();
            // prev_root_hash_Z=BLOCK_id();
            state_Z=STATE_NORMAL;
            stage_is_working=false;
            last_activity_time=0;
            contracts.clear();

        }


        void do_start_block();


        void collectTransactions();
        BLOCK_id execute_block(b_params &b,  const REF_getter<MsgData::HeartBeatREQ> &lc);

        void do_sync(const NODE_id &src_node);

        // bool CheckState(const MsgData::HeartBeatREQ *r, const NODE_id & src_node);

        void calc_fee_rewards_nodes(b_params& t, const REF_getter<MsgData::HeartBeatREQ> &lc);

        BLOCK_id proceed_merkle_on_transaction_pool_hashers(const REF_getter<root_data> &r);
    
        bool verify_block(const REF_getter<MsgData::BlockAcceptedREQ>& lc);

        std::optional<std::string> execute_transaction(const THASH_id &tx_id, b_params &b, const ADDRESS_id &senderAddress,
                         const REF_getter<MsgData::TX> &tx, uint64_t epoch);

        std::optional<std::string> execute_tx_commands(b_params &b, t_params& t, 
             yyjson_val * j_tx);
        void report_mem();

        REF_getter<root_data> root=nullptr;
        REF_getter<IDatabase> db_state=nullptr;
        // REF_getter<DB_history> db_history=nullptr;


        // std::string sqlite_pn;
        // std::string rocksdb_path;
        std::set<msockaddr_in> rpc_addr;
        void logNode(const char* fmt, ...);
        IInstance *iInstance=NULL;

        std::vector<std::string> telnet_data_path;

        blst_cpp::SecretKey my_sk_bls;
        std::string my_sk_ed;

        NODE_id this_node_name;

        std::set<msockaddr_in> web_addr;

        std::string my_sk_bls_env_key;
        std::string my_sk_ed_env_key;
        std::string db_name;

        MsgFactory msgFactory;

        std::optional<std::string> load_contract(const CONTRACT_id& contract);
        std::optional<std::string> execute_contract(const CONTRACT_id& ct, const std::string & method, yyjson_val* params);

        JSRuntime *contract_runtime=NULL;

        void dump(nlohmann::json &j)
        {
            j["msgFactory.registry.size()"]=msgFactory.registry.size();
            j["c_blocks.size()"]=c_blocks.size();
            j["l_blocks.size()"]=l_blocks.size();
            j["cli_leader_info.size()"]=cli_leader_info.size();
            j["syncs.size()"]=syncs.size();

            size_t ft=0;
            for(auto &z: filter_NodeMsgREQ)
            {
                for(auto& x:z.second)
                {
                    for(auto& y:x.second)
                    {
                        ft++;
                    }
                }
            }
            j["filter_NodeMsgREQ"]=ft;
            j["contracts size"]=contracts.size();
        // std::map<BLOCK_id, block_client> c_blocks;
        // std::map<BLOCK_id,block_leader> l_blocks;
        // std::map<BLOCK_id, client_leader_info> cli_leader_info;
        // std::map<BLOCK_id,_sync> syncs;
        // std::map<NODE_id,std::map<int64_t,std::set<int64_t> > > filter_NodeMsgREQ;
        // std::map<CONTRACT_id, REF_getter<contract_rt> > contracts;

        }
            
    };

}

