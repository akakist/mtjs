#ifndef _______________displayzzPLAYBCNODE___H1INV
#define _______________displayzzPLAYBCNODE___H1INV
#include "broadcaster.h"

#include "listenerSimple.h"
#include <map>
#include "Events/System/Run/startServiceEvent.h"

#include "Events/System/Net/httpEvent.h"
#include "listenerSimple.h"
#include "Events/System/Net/webSocketClientEvent.h"




namespace testWSC
{
    class Service:
        public UnknownBase,
        public ListenerSimple,
        public Broadcaster
    {
        bool on_startService(const systemEvent::startService*);
        bool handleEvent(const REF_getter<Event::Base>& e);



    public:
        Service(const SERVICE_id&, const std::string&  nm, IInstance *ins);
        ~Service();


        bool on_RequestIncoming(const httpEvent::RequestIncoming*);
        bool Connected(const webSocketClientEvent::Connected*);
        bool ConnectFailed(const webSocketClientEvent::ConnectFailed*);
        bool Disconnected(const webSocketClientEvent::Disconnected*);
        bool Received(const webSocketClientEvent::Received*);



    public:
        void deinit()
        {
            ListenerSimple::deinit();
        }

        static UnknownBase* construct(const SERVICE_id& id, const std::string&  nm,IInstance* obj)
        {
            return new Service(id,nm,obj);
        }
        // msockaddr_in bindAddr;
        IInstance * iInstance;
        std::string url;

    };

}
#endif

