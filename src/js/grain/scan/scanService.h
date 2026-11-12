#pragma once
#include "listenerBuffered1Thread.h"
#include "broadcaster.h"
#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/timerEvent.h"
#include "Event/bcEvent.h"
#include "IDatabase.h"
#include "Events/System/Net/rpcEvent.h"

namespace Scan
{
    class Service: public UnknownBase, public ListenerBuffered1Thread, public Broadcaster
    {
        bool on_startService(const systemEvent::startService*);
        bool handleEvent(const REF_getter<Event::Base>& e);
        bool ServiceInit(const bcEvent::ServiceInit *e);
        bool ClientMsg(const bcEvent::ClientMsg* e);

        Service(const SERVICE_id&, const std::string& nm, IInstance* ins);
        ~Service();
    public:
        void deinit() { ListenerBuffered1Thread::deinit(); }
        static UnknownBase* construct(const SERVICE_id& id, const std::string& nm, IInstance* obj) { return new Service(id, nm, obj); }

        REF_getter<IDatabase> db_state;
    };
}
