#pragma once

#include "broadcaster.h"

// #include "signedBuffer.h"

#include "listenerBuffered1Thread.h"
#include <map>
#include <rocksdb/db.h>
#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/timerEvent.h"
#include "Event/bcEvent.h"
#include "root_contract.h"
#include "TRANSACTION_id.h"
#include "THASH_id.h"



namespace TxValidator
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

        bool ServiceInit(const bcEvent::ServiceInit *e);
        bool ClientMsg(const bcEvent::ClientMsg*e);
        bool InvalidateRoot(const bcEvent::InvalidateRoot*e);
        bool AddTxREQ(const bcEvent::AddTxREQ*e);



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

        std::map<THASH_id, TRANSACTION_body>  transaction_pool_verified;

        // bool is_working=false;
        REF_getter<root_data> root=NULL;

        REF_getter<bcEvent::ServiceInit> conf=nullptr;



    };

}

