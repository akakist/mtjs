#pragma once

#include <atomic>
#include <functional>
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
#include "md/md_PingREQ.h"
#include "md/md_PingRSP.h"
#include "md/md_BlockAcceptedREQ.h"
#include "md/md_BlockDiffValidateREQ.h"
#include "md/md_BlockDiffValidateRSP.h"
#include "md/md_GetValidateBlockRSPREQ.h"
#include "md/md_GetValidateBlockRSPRSP.h"
#include "md/md_UnlockRoundREQ.h"
#include "md/md_BlockStatusREQ.h"

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

/// Раунд/статус не чаще раза в секунду от прошлого блока: иначе пустые блоки
/// сыплются непрерывно (каждый блок сбрасывает лидера).
#define STATUS_INTERVAL_MKS 1000000
/// tx-аларм: пул пуст — ждём больше, но меньше интервала статуса.
#define TX_EMPTY_TIMEOUT_MKS 400000
/// tx-аларм: пришла tx — короткое окно «дожать» поток.
#define TX_SHORT_TIMEOUT_MKS 20000
/// Страховка sent, если блок не случился (до конца блока sent снимается и так).
#define TX_SENT_TTL_SEC 2.0
/// Лимит tx в блоке.
#define MAX_TXS_PER_BLOCK 200
/// Таймаут задачи async_dl (получение данных по хешам) и TTL кеша данных.
#define DL_TASK_TIMEOUT_SEC 3.0
#define DATA_CACHE_TTL_SEC 300

namespace Node
{
    enum Mode { Full = 0, Lite = 1 };

    enum timers
    {
        TIMER_PERIODIC_CLOCK = 0 ,
        ALARM_START_BLOCK = 1,
        ALARM_do_request_for_tx  = 2,
        TIMER_GetTxREQ = 3,
        ALARM_Ping  = 4,
        // TIMER_GetTxFromLeader,
        ALARM_SYNC_WATCHDOG,
        TIMER_REPORT_MEM,
        TIMER_BlockDiffValidateREQ,
        TIMER_BlockDBStore,
        TIMER_STALE_CLEANUP,
        TIMER_BLOCK_STATUS,   // периодическая рассылка статуса лидером (200 мс)
        // TIMER_BLOCK_EXPIRED,
        ALARM_GetValidateBlockRSP,
        ALARM_DL_TIMEOUT,   // шаг 3: повтор сбора сертификата 2/3
        ALARM_TX_SENT,               // страховка: вернуть tx из sent в пул

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
        bool GetPrevBlockREQ(const bcEvent::GetPrevBlockREQ* e);
        bool on_GetHashedDataREQ(const bcEvent::GetHashedDataREQ* e);
        bool on_GetHashedDataRSP(const bcEvent::GetHashedDataRSP* e);
        bool BlockStatusREQ(const MsgData::BlockStatusREQ* e, const NODE_id& src_node, const route_t& route);
        bool GetPrevBlockRSP(const bcEvent::GetPrevBlockRSP* e);


        bool GetTxRSP(const MsgData::GetTxRSP* r, const NODE_id & src_node, const route_t& route);
        bool ValidateBlockREQ(const MsgData::ValidateBlockREQ* r, const NODE_id & src_node, const route_t& route);
        bool ValidateBlockRSP(const MsgData::ValidateBlockRSP* r, const NODE_id & src_node, const route_t& route);
        bool BlockDBStore(const MsgData::BlockDBStore* r, const NODE_id & src_node, const route_t& route);

        bool BlockDiffValidateREQ(const MsgData::BlockDiffValidateREQ* r, const NODE_id & src_node, const route_t& route);
        // bool BlockDiffValidateREQ_sync(const MsgData::BlockDiffValidateREQ* r, const NODE_id & src_node, const route_t& route);
        bool BlockDiffValidateRSP(const MsgData::BlockDiffValidateRSP* r, const NODE_id & src_node, const route_t& route);

        // шаг 3: запрос/ответ сертификата 2/3 за высоту
        
        ///? Зачем нужен GetValidateBlockRSPREQ, GetValidateBlockRSPRSP
        bool UnlockRoundREQ(const MsgData::UnlockRoundREQ* r, const NODE_id & src_node, const route_t& route);




        bool NodeMsgREQ(const bcEvent::NodeMsgREQ* m);
        bool NodeMsgRSP(const bcEvent::NodeMsgRSP* m);

        bool syncGetGranulesRSP(const bcEvent::syncGetGranulesRSP* m);
        bool syncGetGranulesREQ(const bcEvent::syncGetGranulesREQ*e);



        bool isNodeGreater(const NODE_id& nodeLeft, const NODE_id& nodeRight, const REF_getter<BlockMetaFull>& meta);

        void mark_activity(const std::set<NODE_id>& alive_set, Rollback* roll, TIME_mks block_timestamp);



        void broadcast_MsgEvent_via_broadcaster(const REF_getter<MsgData::Base>& p,  const TreeNode &tree);
        void pass_NodeMsgRSP(const MsgData::Base *e,const route_t& r);



        struct block_leader
        {

            std::set<THASH_id> sent_tx;
            std::map<int/* round */, std::map<int /*subblock*/, std::pair<THASH_id, _db_to_save>>> dumps;           
            // THASH_id offer_hash;   // хеш оффера текущего раунда (для статуса)

            // REF_getter<MsgData::ValidateBlockREQ> offer=nullptr;
            REF_getter<MsgData::BlockStatusREQ> block_status = nullptr;
            // std::map<THASH_id /*hb hash*/, std::map<NODE_id,REF_getter<MsgData::GetTxRSP> > >GetTxRSP_m; /// для подсчета ответивших нод
            // bool is_GetTransactionRSP_m_quorum(const THASH_id& h, Cellable* r, IDatabase* mf);
        

            // std::map<THASH_id /*hb hash*/, std::vector<REF_getter<MsgData::ValidateBlockRSP> > >ValidateBlockRSP_m;
            // bool is_ValidateBlockRSP_m_quorum(const THASH_id& h, BlockMetaFull* mf);

            // std::map<THASH_id /*hb hash*/, std::vector<REF_getter<MsgData::BlockDiffValidateRSP> > >BlockDiffValidateRSP_m;
            // bool is_BlockDiffValidateRSP_m_quorum(const THASH_id& h, Cellable* r, IDatabase* db);

            // std::map<THASH_id, REF_getter<MsgData::BlockDiffValidateREQ> > blockDiffValidateREQ_2;
            // std::set<NODE_id> alive;   // ответившие на GetTxREQ (живой сет) — фильтр рассылки блока
            // bool ping_sent = false;   // пинг высылается один раз на блок
            // bool ping_done = false;   // пинг-стадия завершена (alive финализирован)
            // tx-сбор: аларм старта блока не зависит от кворумов.
            // REF_getter<MsgData::HeartBeatREQ> my_hb;  // hb текущего раунда
            // bool block_started = false;               // do_start_block уже вызван
            // int64_t tx_deadline_mks = 0;              // кап на продление tx-аларма
            // int64_t leader_slot = -1;                 // слот, в котором мы лидер (попытка)

        };
        /// Перевзвести tx-аларм с короткой задержкой, но не позже дедлайна раунда.
        // void armBlockStartAlarm(block_leader& bt, const REF_getter<MsgData::HeartBeatREQ>& hb, double want_secs);
        /// Текущий слот от timestamp прошлого блока (-1 — часы ушли назад).
        // int64_t current_slot();
        _db_to_save db_to_save_Z;



        void do_request_for_transactions(MsgData::HeartBeatREQ* hb, const REF_getter<BlockMetaFull>& meta);

        // struct client_leader_info
        // {
        //     REF_getter<MsgData::HeartBeatREQ> node_leader=nullptr;
        //     route_t leader_route;      // маршрут обратно к лидеру (из GetTxREQ)
        //     bool leader_route_valid=false;
        // };

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
        // std::map<THASH_id, client_leader_info> cli_leader_info;
        std::map<NODE_id,std::map<int64_t,std::set<int64_t> > > filter_NodeMsgREQ;
        /// anti-replay для syncGetGranulesREQ: последний принятый current_time (mks) на узел
        std::map<NODE_id,int64_t> last_sync_req_time;
        std::map<THASH_id, REF_getter<MsgData::TX> >  transaction_pool_of_leader;
        // tx, уже отданные в GetTxRSP: до конца блока не отдаём повторно.
        // Чистится при применении блока (или страховочным алармом).
        std::set<THASH_id> tx_sent;
        // за какие blockInfo мы уже добавили свою подпись в BlockAccepted
        std::set<THASH_id> accepted_signed;

        // ── кеш данных (data plane): hash -> REF<MsgData::Base> ──────────
        // Диффы, att_data, список tx: контентно-адресуемы хешами из blockInfo.
        std::map<THASH_id, std::pair<int64_t,REF_getter <MsgData::Base>> > data_cache;
        
        // задача «получить данные по хешам»; финалайз вызывается там же,
        // где пришёл последний ответ (или по таймауту).
        struct dl_task
        {
            int64_t id = 0;
            std::set<THASH_id> need, have;
            std::vector<std::string> sources;
            size_t next_source = 0;
            std::function<void(bool)> done;
            int64_t deadline_mks = 0;
        };
        std::map<int64_t, dl_task> dl_tasks;
        int64_t dl_task_seq = 0;

        // Статус по высотам: лидер хранит/ретранслирует, ноды подхватывают.
        // std::map<int64_t, REF_getter<MsgData::BlockStatusREQ>> block_status;
        // void mergeBlockStatus(const MsgData::BlockStatusREQ* e, int64_t epoch);
        void broadcastCurrentBlockStatus(bool round_finished);
        bool dl_test_done = false;   // тест-хук MTJS_TEST_DL_FETCH — один раз

        void data_cache_add(const THASH_id &h, const REF_getter<MsgData::Base>& p);
        REF_getter<MsgData::Base> data_cache_get(const THASH_id& h);
        /// Закинуть в кеш диффы/att_data/список tx (их getHash — ключи).
        // void cacheBlockPayloads(const REF_getter<MsgData::attachment_data>& att);
        /// Асинхронно получить данные по хешам из sources; done(true/false).
        void async_dl(const std::vector<THASH_id>& hashes,
                      const std::vector<std::string>& sources,
                      std::function<void(bool)> done);
        void requestNextDlSource(dl_task& t);
        std::string self_ip();
        /// Источники для подгрузки данных субблоков (лидер + enabled-ноды), ip без дублей.
        /// Источники для async_dl — ip подписавших субблок: данные у них точно есть.
        std::vector<std::string> subblock_data_sources(const REF_getter<MsgData::SubBlock>& s, const REF_getter<BlockMetaFull>& mf);
        /// Обработка одного субблока текущего статуса: проверка подписей, стадии
        /// (голос за оффер → кворум комитета → apply диффов → кворум диффов → финал),
        /// догрузка отсутствующих данных через async_dl.
        /// Ищет субблок по offer_hash в актуальном block_status; attempt — защита от зацикливания.
        void process_status_subblock(const THASH_id& offer_hash, int round, const route_t& route, int attempt=1);
        /// Записать финальный блок в БД и переключить prev_block (общий хвост с BlockDBStore).
        void commitFinalBlock(const REF_getter<MsgData::BlockAccepted2REQ>& ba2,
                              const std::vector<THASH_id>& tx_hashes,
                              const std::map<std::string,std::string>& diffs,
                              const REF_getter<MsgData::attachment_data>& att);
        /// Раунд завершён (finished) и все субблоки собраны: слить их дампы в один
        /// батч (поздние гранулы перезаписывают ранние) и сбросить в базу как в BlockStore.
        void finalize_round(int round, const route_t& route);
        std::map<THASH_id/* block */,std::map<int, REF_getter<BlockMetaFull>> >block_meta_full;
        REF_getter<BlockMetaFull> getMetaFull(const THASH_id &meta_root_hash = THASH_id());

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
        // Подписанный нами BlockInfo (замок): не даёт подписать два блока на одной высоте.
        // Хранится в БД под "...signed_block...", подгружается при старте.
        REF_getter<MsgData::BlockInfo> block_lock;
        REF_getter<MsgData::BlockAccepted2REQ> prev_block;

        // ── Шаг 3: сертификат 2/3 за (epoch, root) ──────────────────────────
        // Свои подписанные ValidateBlockRSP: отдаются по GetValidateBlockRSPREQ
        // другому лидеру. Держим 2 последние эпохи — отвечать нужно и после
        // применения блока (часть комитета к моменту сбора уже применила).
        std::map<int64_t, REF_getter<MsgData::ValidateBlockRSP>> my_signed_rsp;
        // наши diff-голоса (ba_hash+sig), чтобы отдавать их после рестарта/в статусе
        std::map<int64_t, REF_getter<MsgData::BlockDiffValidateRSP>> my_diff_rsp;
        // hb чужого лидера, виденный в PingREQ: признак, что на высоте был
        // подписан блок, даже если мы сами ValidateBlockREQ не получали.
        std::map<int64_t, REF_getter<MsgData::HeartBeatREQ>> seen_hb;

        
        void unlockRoundLocal(int64_t epoch, const THASH_id& base);

        int64_t node_start_timestamp=0;
        int64_t clock_lag_us_ = 0;              // тестовый сдвиг часов ноды (мкс, со знаком)
        TIME_mks getNow();                      // time source ноды = wall-clock + clock_lag_us_
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
        THASH_id execute_block(b_params &b,  const REF_getter<MsgData::HeartBeatREQ> &lc, MsgData::ValidateBlockREQ* txs);
        /// Детерминированный слэш: проверить эвиденс двойной подписи и сжечь стейк.


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
        // REF_getter<const bc_node> current_leader();
        // void resetBlockExpired(const NODE_id &n);
        bool on_NodeNotResponded(const bcEvent::NodeNotRespondedREQ *e);
        void collect_granules_since_blocktimestamp(Cellable *c, IDatabase *db, TIME_mks block_timestamp, std::vector<std::pair<std::string,std::string>>&dbdump, int64_t& cnt, const route_t &route);
        void process_offer(const THASH_id & offer, const REF_getter<MsgData::SubBlock>& status, const route_t &route, int round, int subblock);
        // std::map<THASH_id,REF_getter<MsgData::Base>> cache;
        

        void apply_diffs(const THASH_id &att_hash, const route_t& route, int round, int subblock);
        SB_result verify_subblock_sigs(int round, int subblock, const route_t& route, const REF_getter<MsgData::SubBlock>& b,
                               const REF_getter<MsgData::HeartBeatREQ>& hb,
                               const REF_getter<BlockMetaFull>& mf);

    };

}

