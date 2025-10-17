#pragma once
#include <REF.h>
#include <webDumpable.h>
#include <SOCKET_id.h>

#include <epoll_socket_info.h>

#include <broadcaster.h>
#include <msockaddr_in.h>


#include "listenerSimple.h"
#include <IRPC.h>

#include "Events/System/Net/rpcEvent.h"
#include "Events/System/Net/oscarEvent.h"

#include <Events/System/timerEvent.h>
#include <Events/System/Run/startServiceEvent.h>
#include <Events/Tools/webHandlerEvent.h>


namespace RPC
{

    namespace timers
    {
        enum
        {
            CONNECTION_ACTIVITY
        };
    }

    struct outCache: public Refcountable
    {
        RWLock lk;
        std::deque<REF_getter<refbuffer> >  container_lk;


    };
    struct Session: public Refcountable
#ifdef WEBDUMP
        , public WebDumpable
#endif
    {

        SOCKET_id socketId;
        REF_getter<epoll_socket_info> esi;
        outCache outCache_;

        Session(SOCKET_id sockId,const REF_getter<epoll_socket_info>& _esi):socketId(sockId),
            esi(_esi) {}

        std::string wname()
        {
            return std::to_string(CONTAINER(socketId));
        }
        std::string wdump()
        {
            return "";
        }

    };
    struct __sessions
    {


        __sessions()
        {}

        struct subscr {
            RWLock lk;
            std::set<route_t> container_;
        };

        subscr subscribers_mx;

        struct sock2sess
        {
            RWLock lk;
            std::map<SOCKET_id, REF_getter<Session> > container_;

        };
        sock2sess sock2sess_mx;

        struct sa2sess
        {
            RWLock lk;
            std::map<msockaddr_in,REF_getter<Session> > container_;


        };
        sa2sess sa2sess_mx;

        // all
        struct passCache {
            RWLock lk;
            std::map<SOCKET_id,std::deque<REF_getter<refbuffer>>>passCache;
        };
        passCache passCache_mx;


        void clear()
        {
            {
                W_LOCK(sa2sess_mx.lk);
                sa2sess_mx.container_.clear();
            }
            {

                W_LOCK(subscribers_mx.lk);
                subscribers_mx.container_.clear();
            }
            {
                W_LOCK(sock2sess_mx.lk);
                sock2sess_mx.container_.clear();
            }
        }
    public:
    };

    class Service:
        public UnknownBase,
        public ListenerSimple,
        public Broadcaster,
        public IRPC
    {


        SECURE secure;

        ListenerBase* myOscarListener;

        std::set<msockaddr_in>m_bindAddr_main;
        std::set<msockaddr_in>m_bindAddr_reserve;

        struct _shared_Addr
        {
            RWLock lk;
            _shared_Addr()
            {}
            std::set<msockaddr_in> m_internalAddr;
            std::set<msockaddr_in>m_bindAddr_mainSH;
            std::set<msockaddr_in>m_bindAddr_reserveSH;
        };
        _shared_Addr sharedAddr;

    protected:

    public:
        unsigned short getExternalListenPortMain(); // network byte order
        std::set<msockaddr_in> getInternalListenAddrs(); // network byte order

        __sessions sessions;
    public:


        bool on_Connected(const oscarEvent::Connected*);
        bool on_Disconnected(const oscarEvent::Disconnected*e);
        bool on_Disaccepted(const oscarEvent::Disaccepted*e);
        bool on_PacketOnConnector(const oscarEvent::PacketOnConnector*);
        bool on_PacketOnAcceptor(const oscarEvent::PacketOnAcceptor*);
        bool on_NotifyBindAddress(const oscarEvent::NotifyBindAddress*);
        bool on_NotifyOutBufferEmpty(const oscarEvent::NotifyOutBufferEmpty*);
        bool on_Accepted(const oscarEvent::Accepted*);
        bool on_ConnectFailed(const oscarEvent::ConnectFailed*e);


        bool on_startService(const systemEvent::startService*);

        bool on_PassPacket(const rpcEvent::PassPacket*);
        bool on_SendPacket(const rpcEvent::SendPacket*);

        bool on_SubscribeNotifications(const rpcEvent::SubscribeNotifications* E);
        bool on_UnsubscribeNotifications(const rpcEvent::UnsubscribeNotifications* E);
        bool DoListen(const rpcEvent::DoListen* E);


        bool on_TickAlarm(const timerEvent::TickAlarm*);
#ifdef WEBDUMP
        bool on_RequestIncoming(const webHandlerEvent::RequestIncoming*);
#endif



        bool handleEvent(const REF_getter<Event::Base>& e);

        void flushAll();
        void addSendPacket(const REF_getter<Session>&S, const REF_getter<refbuffer> &P);
        void flushOutCache(const REF_getter<Session> & S);


        IInstance* iInstance;
        bool m_isTerminating;


        void deinit()
        {
            ListenerSimple::deinit();
        }

        Service(const SERVICE_id &svs, const std::string&  nm,  IInstance *ifa);
        static UnknownBase* construct(const SERVICE_id& id, const std::string&  nm,IInstance* ifa);

        ~Service();



    };
}



