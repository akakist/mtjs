#pragma once

#include "broadcaster.h"

// #include "signedBuffer.h"

#include "listenerBuffered1Thread.h"
#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/timerEvent.h"
#include "Event/bcEvent.h"
#include "NODE_id.h"
#include "DBH.h"

#define BROADCAST_ACK_TIMEDOUT_SEC 0.2

enum timers
{
    TIMER_BROADCAST_ACK_TIMEDOUT,
};
struct TIMER_BROADCAST_ACK_TIMEDOUT_cookie: public Refcountable
{
    TIMER_BROADCAST_ACK_TIMEDOUT_cookie(): Refcountable("TIMER_BROADCAST_ACK_TIMEDOUT_cookie") {}
    SERVICE_id dstService;
    NODE_id dstName_;
    TreeNode tree;
    std::string msg;
    route_t route;

};

namespace BroadcasterTree
{
    enum timers
    {
    };
    class Service:
        public UnknownBase,
        public ListenerBuffered1Thread,
        public Broadcaster,
        public DBH_feature
    {
        bool NodeMsgRSP(const bcEvent::NodeMsgRSP*);
        bool on_startService(const systemEvent::startService*);
        bool on_timer(const timerEvent::TickTimer*);
        bool on_alarm(const timerEvent::TickAlarm*);
        bool handleEvent(const REF_getter<Event::Base>& e);

        bool ServiceInit(const bcEvent::ServiceInit *e);

        bool BroadcastMessage(const bcEvent::BroadcastMessage*e);
        bool SendToChild(const bcEvent::SendToChild*e, bool fromNetwork);
        bool SendToChildAck(const bcEvent::SendToChildAck*e, bool fromNetwork);

        void logNode(const char* fmt, ...);



        Service(const SERVICE_id&, const std::string&  nm, IInstance *ins);
        ~Service();

        // void make_broadcast_message_to_tree(SERVICE_id dstService,const std::string & msg, const TreeNode& root, const route_t& route);
        void make_broadcast_message_to_tree(SERVICE_id dstService, const NODE_id& node_signer, int64_t node_start_timestamp, int64_t seqId, const std::string& signature, const std::string &msg, const TreeNode &root, const route_t &route);


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

        REF_getter<bcEvent::ServiceInit> conf=nullptr;

    };

}

