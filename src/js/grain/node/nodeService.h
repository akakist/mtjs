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
#define HEART_BEAT_TIMEDOUT_SEC 20
#define HEART_BEAT_INTERVAL_SEC 5

enum State
{
    NORMAL,SYNCING
};
struct _feeCalcers
{
    std::map<std::string /*pk*/,REF_getter<fee_calcer>> calcers;
    REF_getter<fee_calcer> get(const std::string &pk)
    {
        auto it=calcers.find(pk);
        if(it==calcers.end())
        {
            calcers.insert({pk,new fee_calcer});
            it=calcers.find(pk);
            if(it==calcers.end())
                throw CommonError("if(it==calcers.end())");

        }
        return it->second;
    }
    void clear()
    {
        calcers.clear();
    }
};

namespace Node
{
    enum timers
    {
        TIMER_START_HEART_BEAT,
        TIMER_RESTART_BLOCK,
    };
    struct heart_beat_responce2
    {
        BigInt stake;
        msg::heart_beat_rsp rsp;
        heart_beat_responce2()
        {
            stake=0;
        }
    };
    struct heart_beat_node_info
    {
        heart_beat_node_info() {

            // responses.clear();
            clear();

        }
        bool request_for_transactions_sent=false;
        std::string leader_cert;
        std::map<NODE_id,heart_beat_responce2> responses;
        std::set<NODE_id> transaction_responders;

        void clear()
        {
            request_for_transactions_sent=false;
            leader_cert.clear();
            responses.clear();
            transaction_responders.clear();
        }


    };
    struct heart_beat_info
    {
        NODE_id node_leader;
        std::map<NODE_id,heart_beat_node_info> leader_info;
        heart_beat_info()
        {

        }
        void clear()
        {
            node_leader.container.clear();
            leader_info.clear();
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
        // bool ClientTxSubscribeREQ(const bcEvent::ClientTxSubscribeREQ*);
        bool Msg(const bcEvent::Msg*, bool fromNetwork);
        bool MsgReply(const bcEvent::MsgReply*, bool fromNetwork);



        void do_heart_beat();

        bool on_heart_beat(const msg::heart_beat &hb,const std::string &bt_payload, const route_t& route);
        void on_heart_beat_rsp(const msg::heart_beat_rsp& hbr);

        void make_leader_certificate();

        void do_request_for_transactions(const Node::heart_beat_node_info& li);





        struct block
        {
            std::string block_payload;
            std::vector<msg::block_response> responses;
            BigInt stake;
            std::map<NODE_id /*validator*/, blst_cpp::Signature> sigs;



            std::vector<std::pair<NODE_id/*node*/,blst_cpp::Signature> > signs;
            bool executed=false;
            bool block_accepted_sent=false;
            bool heart_bit_sent_on_block_accepted_rsp=false;

            std::map<NODE_id, msg::block_accepted_rsp> acceptors;

        };
        _db_to_save db_to_save_Z;

        time_t last_access_time_hbZ=0; // heart_bit last tick time
        heart_beat_info    heart_beat_store;
        std::string last_leader_cert;

        std::map<THASH_id, TRANSACTION_body>  transaction_pool_of_leader;
        std::map<BLOCK_id,block> blocks;

        struct _prepared_block
        {
            attachment_data att_data;
            BigInt epoch;
            void clear()
            {
                att_data.clear();
                epoch=0;
            }
        };
        _prepared_block prepared_block;
        // void do_client_tx_report(const msg::publish_block &pb);
// 
        void setBlockId(const BLOCK_id& b)
        {
            prev_block_hash=b;
            auto err=db->put_cell("#root_hash#",b.container);
        }
        BLOCK_id prev_block_hash;
        void do_start_block();

        void collectTransactions();
        BLOCK_id execute_block(const REF_getter<root_data> &rt, const BLOCK_id & bl, const std::vector<TRANSACTION_body >& trs, const std::vector<NODE_id> &nodes_in_leader_cert);



        void on_blockResponse(const msg::block_response& br);
        void on_block_accepted_req(const msg::block_accepted_req& ba, const NODE_id& src_node, const route_t& route);


        void do_sync();
        void on_get_blocks_req(const msg::get_blocks_req &r, const route_t& route);
        void on_get_blocks_rsp(const msg::get_blocks_rsp& r);


        void dump_stats(const msg::publish_block&  pb);


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

    };

}

