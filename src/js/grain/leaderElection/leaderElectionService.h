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
#include <thread>
#include <condition_variable>

struct clientTxSubscription
{
    time_t created_at;
    clientTxSubscription():created_at(time(NULL)) {}
};


namespace LeaderElection
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

        // bool AddTx(const bcEvent::AddTx *e);
        bool ServiceInit(const bcEvent::ServiceInit *e);
        // bool GetTransactions(const bcEvent::GetTransactions*e);
        bool InvalidateRoot(const bcEvent::InvalidateRoot*e);
        // bool ClientMsg(const bcEvent::ClientMsg*e);
        // bool ClientTxSubscribeREQ(const bcEvent::ClientTxSubscribeREQ*);
        // bool StreamBlock(const bcEvent::StreamBlock*);
        




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


        std::map<route_t,clientTxSubscription> clientTxSubscriptions;

    };

}

