#pragma once

#include "broadcaster.h"


#include "listenerBuffered1Thread.h"
#include <rocksdb/db.h>
#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/timerEvent.h"
#include "Event/bcEvent.h"
#include "root_contract.h"



namespace GrainWriter
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
        bool InvalidateRoot(const bcEvent::InvalidateRoot*e);
        bool ClientMsg(const bcEvent::ClientMsg*e);

        bool WriteGranules(const bcEvent::WriteGranules *);



        Service(const SERVICE_id&, const std::string&  nm, IInstance *ins);
        ~Service();

        _db_to_save db_to_save;
        REF_getter<IDatabase> db_state;

        int snapshot_modulus=1000;
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


        REF_getter<root_data> root=NULL;

        REF_getter<bcEvent::ServiceInit> conf=nullptr;



    };

}

