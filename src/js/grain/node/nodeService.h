#pragma once

#include <atomic>
#include "broadcaster.h"
#include "QUORUM.h"
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
#include "THASH_id.h"
#include "NODE_id.h"
#include "db_to_save.h"
#include "blst_cp.h"
#include "md/md_ConfirmLeaderRSP.h"

#include "md/md_HeartBeatRSP.h"
#include "md/md_BlockAcceptedREQ.h"
#include "md/md_BlockAccepted2REQ.h"
#include "md/md_ValidateBlockRSP.h"
#include "md/md_ValidateBlockREQ.h"
#include "md/md_GetTxREQ.h"
#include "md/md_GetTxRSP.h"
#include "md/md_BlockAcceptedREQ.h"
#include "md/md_BlockDiffValidateREQ.h"
#include "md/md_BlockDiffValidateRSP.h"

#include "md/md_ConfirmLeaderREQ.h"
#include "md/md_ConfirmLeaderRSP.h"
#include "md/md_LcEnvelopeREQ.h"
#include "blockMeta.h"


#include "t_params.h"
#include "contract_rt.h"
#include "ContractHost.h"
#include "cells.h"

std::set<NODE_id> getValidators(TIME_mks block_timestamp, IDatabase* db);

// #define HB_TIME_WINDOW 2
#define HB_SEND_TIMEOUT 4
#define HB_VERIFY_TIMEOUT 5
#define SLOT_MKS 3000000

/// Мёртвая зона в конце слота (100 мс): лидера в ней нет.
/// Нужна, чтобы на правой границе слота ноды с рассинхроном часов
/// не считали лидером разных участников и не устраивали гонку.
#define NO_LEADER_FRAME_MKS 100000

/// Idle-таймаут синка: если за это время не было ни одного акта активности
/// (запрос или пачка гранул) — синк прибивается, root поднимается из БД.
#define SYNC_WATCHDOG_SEC 5.0

namespace Node
{
    enum Mode { Full = 0, Lite = 1 };

    enum timers
    {
        TIMER_PERIODIC_CLOCK,
        TIMER_GetTxRSP,
        TIMER_GetTxFromLeader,
        ALARM_SYNC_WATCHDOG,
        TIMER_REPORT_MEM,
        TIMER_BlockDiffValidateREQ,
        TIMER_BlockDBStore,
        TIMER_STALE_CLEANUP,
        TIMER_BLOCK_EXPIRED,

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



        Service(const SERVICE_id&, const std::string& nm, IInstance *ins, Mode mode);
        ~Service();


        bool on_CommandEntered(const telnetEvent::CommandEntered*);

    public:
        void deinit()
        {
            ListenerBuffered1Thread::deinit();
        }

        static UnknownBase* constructFull(const SERVICE_id& id, const std::string& nm, IInstance* obj) {
            return new Service(id, nm, obj, Full);
        }
        static UnknownBase* constructLite(const SERVICE_id& id, const std::string& nm, IInstance* obj) {
            return new Service(id, nm, obj, Lite);
        }

        bool on_RequestIncoming(const webHandlerEvent::RequestIncoming* e);
        bool AddTxREQ(const bcEvent::AddTxREQ *e);
        bool ClientMsg(const bcEvent::ClientMsg *e);

        bool RequestIncoming(const httpEvent::RequestIncoming* e);
        bool PutTxREQ(const bcEvent::PutTxREQ* e);


        // bool HeartBeatRSP(const MsgData::HeartBeatRSP* r, const NODE_id & src_node, const route_t& route);;
        bool GetTxREQ(const MsgData::GetTxREQ* r, const NODE_id & src_node, const route_t& route);
        bool GetTxRSP(const MsgData::GetTxRSP* r, const NODE_id & src_node, const route_t& route);
        bool ValidateBlockREQ(const MsgData::ValidateBlockREQ* r, const NODE_id & src_node, const route_t& route);
        bool ValidateBlockRSP(const MsgData::ValidateBlockRSP* r, const NODE_id & src_node, const route_t& route);
        bool BlockDBStore(const MsgData::BlockDBStore* r, const NODE_id & src_node, const route_t& route);

        bool BlockDiffValidateREQ(const MsgData::BlockDiffValidateREQ* r, const NODE_id & src_node, const route_t& route);
        // bool BlockDiffValidateREQ_sync(const MsgData::BlockDiffValidateREQ* r, const NODE_id & src_node, const route_t& route);
        bool BlockDiffValidateRSP(const MsgData::BlockDiffValidateRSP* r, const NODE_id & src_node, const route_t& route);

        // bool ConfirmLeaderREQ(const MsgData::ConfirmLeaderREQ* m, const NODE_id & src_node, const route_t& route);
        // bool ConfirmLeaderRSP(const MsgData::ConfirmLeaderRSP* m, const NODE_id & src_node, const route_t& route);



        bool NodeMsgREQ(const bcEvent::NodeMsgREQ* m);
        bool NodeMsgRSP(const bcEvent::NodeMsgRSP* m);

        bool syncGetGranulesRSP(const bcEvent::syncGetGranulesRSP* m);
        bool syncGetGranulesREQ(const bcEvent::syncGetGranulesREQ*e);



        bool isNodeGreater(const NODE_id& nodeLeft, const NODE_id& nodeRight, const REF_getter<BlockMetaFull>& meta);

        void fix_missed_rounds(const std::set<NODE_id>& node_validators, time_t block_timestamp);



        void broadcast_MsgEvent_via_broadcaster(const REF_getter<MsgData::Base>& p,  const TreeNode &tree);
        void pass_NodeMsgRSP(const MsgData::Base *e,const route_t& r);



        struct block_leader
        {

            std::map<THASH_id /*hb hash*/, std::map<NODE_id,REF_getter<MsgData::GetTxRSP> > >GetTxRSP_m; /// для подсчета ответивших нод
            bool is_GetTransactionRSP_m_quorum(const THASH_id& h, Cellable* r, IDatabase* mf);


            std::map<THASH_id /*hb hash*/, std::vector<REF_getter<MsgData::ValidateBlockRSP> > >ValidateBlockRSP_m;
            bool is_ValidateBlockRSP_m_quorum(const THASH_id& h, BlockMetaFull* mf);

            std::map<THASH_id /*hb hash*/, std::vector<REF_getter<MsgData::BlockDiffValidateRSP> > >BlockDiffValidateRSP_m;
            bool is_BlockDiffValidateRSP_m_quorum(const THASH_id& h, Cellable* r, IDatabase* db);

            std::map<THASH_id, REF_getter<MsgData::BlockDiffValidateREQ> > blockDiffValidateREQ_2;

        };
        _db_to_save db_to_save_Z;

        void dump(nlohmann::json& j);


        void do_request_for_transactions(MsgData::HeartBeatREQ* hb, const REF_getter<BlockMetaFull>& meta);

        struct client_leader_info
        {
            REF_getter<MsgData::HeartBeatREQ> node_leader=nullptr;
        };

        /// Синк идёт: гейт для всех обработчиков консенсуса.
        std::atomic<bool> sync_in_progress{false};
        // следующий запрос синка — с since=0 (полное дерево).
        // Нужен, когда слияние по дельте не сходится: у клиента не хватает
        // старых ячеек, и сервер их больше не пришлёт (фильтр > since).
        std::atomic<bool> sync_full{false};
        // дерево собирается с нуля (полный синк): старые/удалённые листья
        // не должны попасть в новое дерево.
        std::atomic<bool> sync_fresh_root{false};
        // compact_before_ts на момент старта синка. По нему на финальной пачке
        // понимаем, нужно ли применять правило компакции (порог мог приехать
        // в промежуточной пачке, и тогда локальное значение уже новое).
        TIME_mks sync_old_cutoff;
        /// Продлить/снять watchdog синка (перевзводится на каждой активности).
        void sync_watchdog_arm();
        void sync_watchdog_stop();

        std::map<THASH_id,block_leader> l_blocks;
        std::map<THASH_id, client_leader_info> cli_leader_info;
        std::map<NODE_id,std::map<int64_t,std::set<int64_t> > > filter_NodeMsgREQ;
        /// anti-replay для syncGetGranulesREQ: последний принятый current_time (mks) на узел
        std::map<NODE_id,int64_t> last_sync_req_time;
        std::map<THASH_id, REF_getter<MsgData::TX> >  transaction_pool_of_leader;
        std::map<THASH_id/* block */, REF_getter<BlockMetaFull>> block_meta_full;
        REF_getter<BlockMetaFull> getMetaFull();

        THASH_id prev_root_hash_Z()
        {
            if(prev_block.valid())
                return prev_block->blockAcceptedREQ->blockInfo->new_root_hash1;
            THASH_id r;
            r.container="";
            return r;
        }
        int64_t epoch_current()
        {
            if(prev_block.valid())
                return prev_block->blockAcceptedREQ->blockInfo->heart_beat->new_epoch+1;
            return 0;

        }
        REF_getter<MsgData::BlockAccepted2REQ> prev_block;
        int64_t node_start_timestamp=0;
        int64_t seqId2=0;
        void clear()
        {
            l_blocks.clear();
            block_meta_full.clear();
            transaction_pool_of_leader.clear();
            prev_block=NULL;

        }


        void do_start_block(MsgData::HeartBeatREQ* hb);


        std::vector<REF_getter<MsgData::TX>>  collectTx();
        THASH_id execute_block(b_params &b,  const REF_getter<MsgData::HeartBeatREQ> &lc);


        void calc_fee_rewards_nodes(b_params& t, const REF_getter<MsgData::HeartBeatREQ> &lc);

        bool verify_block_all(const REF_getter<MsgData::BlockAccepted2REQ>& lc);

        std::optional<std::string> execute_transaction(const THASH_id &tx_id, b_params &b, const ADDRESS_id &senderAddress,
                const REF_getter<MsgData::TX> &tx, int64_t epoch);

        std::optional<std::string> execute_tx_commands(b_params &b, t_params& t,
                yyjson_val * j_tx, std::set<REF_getter<data_base>> * deleted_granules);



        REF_getter<IDatabase> db_state=nullptr;


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



        REF_getter<ContractHost> contractHost=nullptr;
        REF_getter<Cellable> root = nullptr;


        std::optional<std::string> execute_mint(yyjson_val *params, b_params & b,t_params &t,   int seqId);
        std::optional<std::string> execute_transfer(yyjson_val *params, b_params &,t_params &t,  int seqId);
        std::optional<std::string> execute_node_create(yyjson_val *params, b_params &,t_params &t,   int seqId);
        std::optional<std::string> execute_node_update(yyjson_val *params, b_params &,t_params &t,   int seqId);
        std::optional<std::string> execute_node_stake(yyjson_val *params, b_params &,t_params &t,  int seqId);
        std::optional<std::string> execute_unstake_node(yyjson_val *params, b_params &,t_params &t, int seqId);
        std::optional<std::string> execute_node_enable(yyjson_val *params, b_params &,t_params &t,  int seqId);
        std::optional<std::string> execute_node_disable(yyjson_val *params, b_params &,t_params &t,  int seqId);
        std::optional<std::string> execute_contract_deploy(yyjson_val *params, b_params &,t_params &t,  int seqId);
        std::optional<std::string> execute_contract_update(yyjson_val *params, b_params &,t_params &t,  int seqId);
        std::optional<std::string> execute_vote(yyjson_val *params, b_params &,t_params &t,  int seqId);
        std::optional<std::string> execute_gc(yyjson_val* params, b_params& b, t_params& t, int seqId, _db_to_save& dbd);
        // компакция: физически удаляет тумбстоуны bc_deleted с ts <= cutoff.
        // Удаления локальные (в diff не идут) — применяются после коммита блока.
        std::optional<std::string> execute_compact(yyjson_val* params, b_params& b, t_params& t, int seqId);
        std::vector<std::string> local_deletes;   // id, удаляемые из RocksDB после коммита
        // deterministic post-order prune: удаляет bc_deleted с ts <= cutoff и пустые узлы.
        // Один и тот же обход используется исполнением компакции и догоном синка.
        // диагностика: дамп дерева (MTJS_TREE_DUMP=1)
        void dump_tree(Cellable* c, IDatabase* db, const char* tag, int depth, int maxdepth);
        void prune_tombstones(Cellable* node, IDatabase* db, TIME_mks cutoff,
                              int64_t& remaining, std::vector<std::string>& out,
                              TIME_mks block_ts, Rollback* roll);
        std::optional<std::string> execute_create_address(yyjson_val* params, b_params& b, t_params& t, int seqId);
        std::optional<std::string> execute_add_address_key(yyjson_val* params, b_params& b, t_params& t, int seqId);
        std::optional<std::string> execute_remove_address_key(yyjson_val* params, b_params& b, t_params& t, int seqId);

        std::optional<std::string> gc_activity_apply(
                const REF_getter<Cellable>& cc, TIME_mks now, t_params& t, b_params& b, _db_to_save& dbd,
                int& applied_granule, int& applied_contract, int& applied_node, int& applied_address);



        Mode mode_ = Full;
        std::string uplink_http_addr_;                    // lite
        std::set<msockaddr_in> http_granule_addr_;      // full

        bool get_stale(const std::string& hash, std::string& out);

        struct StaleEntry { std::string body; int64_t ts; };
        std::map<std::string, StaleEntry> stale_cache;  // hash -> {body, ts}
        void cleanup_stale(int64_t now);
        static constexpr int64_t STALE_TTL_SEC = 1 * 60;

        bool on_LiteSetUplinkREQ(const bcEvent::LiteSetUplinkREQ* e);
        /// Лидер текущего слота. nullptr — лидера нет:
        /// последние NO_LEADER_FRAME_MKS слота, пустой vne, либо нет prev_block.
        REF_getter<bc_node> current_leader();
        void resetBlockExpired(const NODE_id &n);
        void collect_granules_since_blocktimestamp(Cellable *c, IDatabase *db, TIME_mks block_timestamp, std::vector<std::pair<std::string,std::string>>&dbdump, int64_t& cnt, const route_t &route);


    };

}

