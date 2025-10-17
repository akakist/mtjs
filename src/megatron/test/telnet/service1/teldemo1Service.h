#pragma once
#include "broadcaster.h"



#include "logging.h"
#include "listenerBuffered1Thread.h"
#include "DBH.h"
#include "tools_mt.h"
#include <map>
#include <deque>



#include "Real.h"
#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/timerEvent.h"

#include "Events/System/Net/httpEvent.h"
#include "Events/System/Net/rpcEvent.h"
#include "Events/Ping.h"
#include "Events/Tools/telnetEvent.h"
#include "version_mega.h"
namespace teldemo1
{
    class Service:
        public UnknownBase,
        public ListenerBuffered1Thread,
        public Broadcaster
    {
        bool on_startService(const systemEvent::startService*);
        bool handleEvent(const REF_getter<Event::Base>& e);



    public:
        Service(const SERVICE_id&, const std::string&  nm, IInstance *ins);
        ~Service();


        bool on_CommandEntered(const telnetEvent::CommandEntered *e);

        std::deque<std::string> sampleDeque;

    public:
        void deinit()
        {
            ListenerBuffered1Thread::deinit();
        }

        static UnknownBase* construct(const SERVICE_id& id, const std::string&  nm,IInstance* obj)
        {
            return new Service(id,nm,obj);
        }


    };

}

