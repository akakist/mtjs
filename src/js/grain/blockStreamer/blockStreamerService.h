#pragma once

#include "broadcaster.h"

// #include "signedBuffer.h"

#include "listenerBuffered1Thread.h"
#include <map>
#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/timerEvent.h"
#include "Event/bcEvent.h"


enum BlockStreamerTimers { TIMER_CLEAN_SUBSCRIPTIONS = 1000 };

struct clientTxSubscription : public Refcountable
{
    time_t created_at;
    time_t ttl;
    route_t backRoute;
    clientTxSubscription(const route_t& br, time_t _ttl)
        : Refcountable("clientTxSubscription"), created_at(time(NULL)), ttl(_ttl), backRoute(br) {}
};

struct txWaitSubscription : public Refcountable
{
    time_t created_at;
    time_t ttl;
    route_t backRoute;
    txWaitSubscription(const route_t& br, time_t _ttl)
        : Refcountable("txWaitSubscription"), created_at(time(NULL)), ttl(_ttl), backRoute(br) {}
};
namespace BlockStreamer
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
        bool ClientTxSubscribeREQ(const bcEvent::ClientTxSubscribeREQ*);
        bool StreamBlock(const bcEvent::StreamBlock*);





        Service(const SERVICE_id&, const std::string&  nm, IInstance *ins);
        ~Service();



    public:
        std::vector<REF_getter<clientTxSubscription>> allBlocksSubscriptions;
        std::map<THASH_id, std::vector<REF_getter<txWaitSubscription>>> txWaitSubscriptions;

        bool ClientTxWaitREQ(const bcEvent::ClientTxWaitREQ*);
        bool ClientTxUnsubscribeREQ(const bcEvent::ClientTxUnsubscribeREQ*);
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


        // std::map<route_t,clientTxSubscription> clientTxSubscriptions;

    };

}

