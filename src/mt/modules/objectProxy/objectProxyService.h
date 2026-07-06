#pragma once
#include "IUtils.h"
#include "pollable.h"
#include <unknown.h>
#include <broadcaster.h>
#include <IObjectProxy.h>
#include <listenerPolled.h>
#include <Events/System/Run/startServiceEvent.h>
#include <listenerBuffered1Thread.h>
#include "commonError.h"

class ObjectHandler;

namespace ObjectProxy
{
    class Base
    {
    public:

    };
    class Polled:
        public UnknownBase,
        public Broadcaster,
        public IObjectProxyPolled,
        public ListenerPolled,
        public Base,
        public pollable
    {
    public:

        Mutex __m_lock;
        std::set<void*> __mx_handlers;

        void sendObjectRequest(const msockaddr_in & dstHost, const SERVICE_id & dstService, const REF_getter<Event::Base>& e);
        void sendObjectRequest(const SERVICE_id & dstService, const REF_getter<Event::Base>& e);
        bool handleEvent(const REF_getter<Event::Base>& e);

        void addObjectHandler(ObjectHandlerPolled* h);
        void removeObjectHandler(ObjectHandlerPolled* h);

        void deinit()
        {
            ListenerPolled::deinit();
        }


        bool on_startService(const systemEvent::startService*)
        {
            return true;
        }

        Polled(const SERVICE_id &svs, const std::string& nm,IInstance* ifa):
            UnknownBase(nm),
            Broadcaster(ifa),
            ListenerPolled(nm,svs)
        {
            iUtils->addPollable(this);
        }

        ~Polled()
        {
        }
        static UnknownBase* construct(const SERVICE_id& id, const std::string&  nm,IInstance* ifa)
        {
            XTRY;
            return new Polled(id,nm,ifa);
            XPASS;
        }

        void poll()
        {
            ListenerPolled::poll();
        }
    };
    class Threaded:
        public UnknownBase,
        public Broadcaster,
        public IObjectProxyThreaded,
        public ListenerBuffered1Thread,
        public Base
    {
    public:
        Mutex __m_lock;
        std::set<void*> __mx_handlers;

        void addObjectHandler(ObjectHandlerThreaded* h);
        void removeObjectHandler(ObjectHandlerThreaded* h);

        void sendObjectRequest(const msockaddr_in & dstHost, const SERVICE_id & dstService, const REF_getter<Event::Base>& e);
        void sendObjectRequest(const SERVICE_id & dstService, const REF_getter<Event::Base>& e);
        bool handleEvent(const REF_getter<Event::Base>& e);
        bool on_startService(const systemEvent::startService*) {
            return true;
        }



    public:

        Threaded(const SERVICE_id &svs, const std::string& nm,IInstance* ifa):UnknownBase(nm),
            Broadcaster(ifa),
            ListenerBuffered1Thread(nm,svs)
        {
        }

        ~Threaded()
        {
        }
        void deinit()
        {
            ListenerBuffered1Thread::deinit();
        }

        static UnknownBase* construct(const SERVICE_id& id, const std::string&  nm,IInstance* ifa)
        {
            XTRY;
            return new Threaded(id,nm,ifa);
            XPASS;
        }
    };

}
