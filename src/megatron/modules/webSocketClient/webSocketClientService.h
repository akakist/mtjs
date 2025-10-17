#pragma once
#include "unknown.h"
#include "SERVICE_id.h"
#ifndef _MSC_VER
#include <sys/time.h>
#endif
#include "IInstance.h"
#include "listenerBuffered1Thread.h"
#include "broadcaster.h"


#include "Events/System/Run/startServiceEvent.h"
// #include "Events/System/Net/rpcEvent.h"
#include "Events/System/Net/webSocketClientEvent.h"
#include "Events/System/Net/socketEvent.h"
#include "url.hpp"
namespace webSocketClient
{
    struct state: public Refcountable
    {
        bool awaiting_handshake=false;
        Url url;
        std::string url_original;

    };

    class Service:
        public UnknownBase,
        public ListenerBuffered1Thread,
        public Broadcaster

    {
        bool on_startService(const systemEvent::startService*);
        bool Connect(const webSocketClientEvent::Connect*);
        bool Send(const webSocketClientEvent::Send*);
        bool handleEvent(const REF_getter<Event::Base>& e);
        // bool on_IncomingOnAcceptor(const rpcEvent::IncomingOnAcceptor*);
        bool Connected( socketEvent::Connected*);
        bool Disconnected(const socketEvent::Disconnected*);
        bool ConnectFailed(const socketEvent::ConnectFailed*);
        bool NotifyOutBufferEmpty(const socketEvent::NotifyOutBufferEmpty*);
        bool StreamRead(const socketEvent::StreamRead*);


        Service(const SERVICE_id&, const std::string&  nm, IInstance *ifa);
        ~Service();


        std::set<route_t> m_subscribers;
        //std::map<std::string,std::string> m_cache;

    public:
        void deinit()
        {
            ListenerBuffered1Thread::deinit();
        }
        static UnknownBase* construct(const SERVICE_id& id, const std::string&  nm,IInstance* ifa)
        {
            XTRY;
            return new Service(id,nm,ifa);
            XPASS;
        }

    private:
        std::map<SOCKET_id,REF_getter<state>> states;

    };
};
