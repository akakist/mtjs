#ifndef _______________displayzzPLAYBCNODE___H1INV
#define _______________displayzzPLAYBCNODE___H1INV
#include "broadcaster.h"



#include "listenerBuffered1Thread.h"
#include <map>



#include "Events/System/Run/startServiceEvent.h"

#include "Events/System/Net/httpEvent.h"
#include "Events/Ping.h"
#define SESSION_ID  "session_id"
enum TIMERS
{
    TIMER_PUSH_NOOP=1
};
namespace prodtestWebServer
{
    class Session:
        public Refcountable
    {
    public:
        int sessionId;
        REF_getter<epoll_socket_info> esi;
        REF_getter<HTTP::Request> req;


        Session(int sid, const REF_getter<HTTP::Request> &_req,const REF_getter<epoll_socket_info> &_esi): sessionId(sid),
            req(_req),
            esi(_esi)
        {
        }
        ~Session() {}

    };



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


        bool on_RequestIncoming(const httpEvent::RequestIncoming*);
        bool on_AddTaskRSP(const prodtestEvent::AddTaskRSP*e);



    public:
        void deinit()
        {
            ListenerBuffered1Thread::deinit();
        }

        static UnknownBase* construct(const SERVICE_id& id, const std::string&  nm,IInstance* obj)
        {
            return new Service(id,nm,obj);
        }

        REF_getter<prodtestWebServer::Session> create_session(const REF_getter<HTTP::Request>& req, HTTP::Response& resp, const REF_getter<epoll_socket_info> &_esi);
        REF_getter<prodtestWebServer::Session> get_session(int session_id);
        void remove_session(int session_id);

        std::map<int,REF_getter<Session> > sessions;


        msockaddr_in bindAddr;
        std::string prodtestServerAddr;
        IInstance *iInstance;

        int cnt2=0;


    };

}
#endif

