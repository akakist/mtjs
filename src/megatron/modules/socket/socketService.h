#pragma once


#include <REF.h>
#include <SOCKET_id.h>
#include <epoll_socket_info.h>

#include <unknown.h>
#include <listenerBuffered1Thread.h>
#include <broadcaster.h>
#include <Events/System/Net/socketEvent.h>
#include <Events/System/Run/startServiceEvent.h>
#include <Events/System/timerEvent.h>
#include <Events/Tools/webHandlerEvent.h>

namespace SocketIO
{
    struct SocketsContainerForSocketIO
        :public Refcountable
    {

    private:
        std::map<SOCKET_id,REF_getter<epoll_socket_info> > container_;
    public:
        REF_getter<NetworkMultiplexor> multiplexor_;
        RWLock lk;
        SocketsContainerForSocketIO(): multiplexor_(new NetworkMultiplexor)
        {}
        void remove(const SOCKET_id& sid)
        {
            W_LOCK(lk);
            container_.erase(sid);

        }
        std::map<SOCKET_id,REF_getter<epoll_socket_info> > getContainer()
        {
            R_LOCK(lk);
            return container_;
        }
        size_t count(const SOCKET_id& sid)
        {
            R_LOCK(lk);
            return container_.count(sid);
        }
        REF_getter<epoll_socket_info> getOrNull(const SOCKET_id& sid)
        {
            R_LOCK(lk);
            auto z=container_.find(sid);
            if(z==container_.end())
                return NULL;
            return z->second;
        }

        void add(const REF_getter<epoll_socket_info>& esi)
        {
            W_LOCK(lk);
            container_.insert(std::make_pair(esi->id_,esi));
        }

        void clear()
        {
            multiplexor_=NULL;
        }

    private:
    };

    class Service:
        public UnknownBase,
        public Broadcaster,
        public ListenerBuffered1Thread

    {
    private:

        struct PTHSOCKS
        {
            RWLock lock;
            std::vector<REF_getter<SocketsContainerForSocketIO> > socks_pollers_;
            std::deque<REF_getter<SocketsContainerForSocketIO> > for_threads_;
            REF_getter<SocketsContainerForSocketIO> getPoller(SocketIO::Service* s)
            {
                R_LOCK(lock);
                if(socks_pollers_.size()!=s->n_workers_)
                    throw CommonError("if(m_socks_pollers.size()!=s->n_workers)");
                {
                    return socks_pollers_[rand()%socks_pollers_.size()];
                }
            }
        };
        PTHSOCKS m_io_socks_pollers_;
        std::vector<pthread_t> pthread_id_worker_;
        int64_t listen_backlog_;
        int n_workers_;
        /// конфигурационный параметр
        int epoll_timeout_millisec_;


        void closeSocket(const REF_getter<epoll_socket_info>&esi, const char *reason, int errNo, const REF_getter<SocketsContainerForSocketIO> &MS);
        bool on_AddToListenTCP(const socketEvent::AddToListenTCP*);
        bool on_AddToConnectTCP(const socketEvent::AddToConnectTCP*);
        bool on_RequestIncoming(const webHandlerEvent::RequestIncoming*);
        void onEPOLLIN(epoll_socket_info* esi, const REF_getter<SocketsContainerForSocketIO> &MS);
        void onEPOLLIN_STREAMTYPE_LISTENING(epoll_socket_info *esi, const REF_getter<SocketsContainerForSocketIO> &MS);
        void onEPOLLIN_STREAMTYPE_CONNECTED_or_STREAMTYPE_ACCEPTED(epoll_socket_info *esi, const REF_getter<SocketsContainerForSocketIO> &MS);



        void onEPOLLOUT(epoll_socket_info* esi, const REF_getter<SocketsContainerForSocketIO> &MS);
        void onEPOLLERR(epoll_socket_info* esi, const REF_getter<SocketsContainerForSocketIO> &MS);
        bool  handleEvent(const REF_getter<Event::Base>&);
        void handle_accepted1(const SOCKET_fd &neu_fd, const REF_getter<epoll_socket_info> esi, const REF_getter<SocketsContainerForSocketIO>& MS, const msockaddr_in &remote_sa);


        IInstance* iInstance;
        bool isTerminating_;

    public:
        static UnknownBase *construct(const SERVICE_id& id, const std::string&  nm, IInstance* ifa);
        Service(const SERVICE_id& id, const std::string& nm,  IInstance *ifa);
        ~Service();
        void deinit()
        {
            ListenerBuffered1Thread::deinit();
        }

    protected:
        static void *worker__(void*);
        void worker();

    private:
        void initSSL(const std::string& cert_pn, const std::string& key_pn);
        REF_getter<SECURE_CONTEXT> secure_context=nullptr;
    };
};

