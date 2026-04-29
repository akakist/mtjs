#pragma once

#include "broadcaster.h"


#include "listenerBuffered1Thread.h"
#include <map>
#include <rocksdb/db.h>
#include "Events/System/Run/startServiceEvent.h"
#include "Events/Tools/telnetEvent.h"
#include "Events/Tools/webHandlerEvent.h"
#include "Events/System/Net/httpEvent.h"
#include "Events/System/timerEvent.h"
#include "Event/bcEvent.h"
#include "root_contract.h"
#include "msg.h"
#include "bigint.h"
#include "NODE_id.h"
#include <thread>

struct clientTxSubscription
{
    time_t created_at;
    clientTxSubscription():created_at(time(NULL)) {}
};

enum timers
{
    TIMER_START_HEART_BEAT,
    TIMER_RESTART_BLOCK,
};
#define HEART_BEAT_TIMEDOUT_SEC 20
#define HEART_BEAT_INTERVAL_SEC 5

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


namespace TransactionCollector
{
    enum timers
    {
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

        bool Msg(const bcEvent::Msg *e);
        bool MsgReply(const bcEvent::MsgReply *e);
        bool ServiceInit(const bcEvent::ServiceInit *e);
        // bool GetTransactions(const bcEvent::GetTransactions*e);
        bool InvalidateRoot(const bcEvent::InvalidateRoot*e);
        bool StartElection(const bcEvent::StartElection*e);
        // bool ClientTxSubscribeREQ(const bcEvent::ClientTxSubscribeREQ*);
        bool StartCollector(const bcEvent::StartCollector*);
        

        void logNode(const char* fmt, ...);
        bool on_heart_beat(const msg::heart_beat &h,const std::string &heart_beat_payload, const route_t& route);
        void on_heart_beat_rsp(const msg::heart_beat_rsp& hbr);

        void make_leader_certificate();
        bool MsgReply(const bcEvent::MsgReply* e);


        Service(const SERVICE_id&, const std::string&  nm, IInstance *ins);
        ~Service();



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

        // std::map<THASH_id, TRANSACTION_body>  transaction_pool_verified;

        REF_getter<root_data> root=NULL;

        REF_getter<bcEvent::ServiceInit> conf=nullptr;

        std::map<THASH_id, TRANSACTION_body>  transaction_pool_of_leader;


        std::map<route_t,clientTxSubscription> clientTxSubscriptions;

        heart_beat_info    heart_beat_store;
        time_t last_access_time_hbZ=0; // heart_bit last tick time
        BLOCK_id prev_block_hash;



    };

}

