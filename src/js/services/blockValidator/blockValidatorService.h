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


enum State
{
    NORMAL,SYNCING
};

namespace BlockValidator
{
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










    };

}

