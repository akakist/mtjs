#pragma once
#include "broadcaster.h"
#include "REF.h"
#include "listenerBuffered1Thread.h"
#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/timerEvent.h"
#include "Events/DFS/referrerEvent.h"
#include "Events/DFS/capsEvent.h"
#include "CapsAlgorithm.h"
#include "timerHelper.h"
struct ServiceNode;

#define GET_SERVICE_NODE_LIMIT 50



namespace dfsCaps
{


    class Service:
        public UnknownBase,
        public ListenerBuffered1Thread,
        public Broadcaster,
        public TimerHelper
    {
        bool handleEvent(const REF_getter<Event::Base>& e);
        bool on_startService(const systemEvent::startService*);
        real Distance(const std::pair<real,real> & c1,const std::pair<real,real> & c2);
        bool on_ToplinkDeliverREQ(const dfsReferrerEvent::ToplinkDeliverREQ* e);
        bool on_TickAlarm(const timerEvent::TickAlarm*);
#ifdef WEBDUMP
        bool on_RequestIncoming(const webHandlerEvent::RequestIncoming*);
#endif

        bool on_GetReferrersREQ(const dfsCapsEvent::GetReferrersREQ* e, const dfsReferrerEvent::ToplinkDeliverREQ *e_toplink);
        bool on_RegisterMyRefferrerNodeREQ(const dfsCapsEvent::RegisterMyRefferrerNodeREQ* e);

        bool on_GetCloudRootsREQ(const dfsCapsEvent::GetCloudRootsREQ* e, const dfsReferrerEvent::ToplinkDeliverREQ *e_toplink);
        bool on_RegisterCloudRoot(const dfsCapsEvent::RegisterCloudRoot* e);

        IInstance* iInstance;

        CapsAlgorithm referrer_nodes;
        CapsAlgorithm cloud_roots;
    public:
        void deinit()
        {
            ListenerBuffered1Thread::deinit();
        }

        Service(const SERVICE_id &svs, const std::string&  nm,IInstance* _ifa);
        static UnknownBase* construct(const SERVICE_id& id, const std::string&  nm, IInstance* _ifa);
        ~Service();

    };
}


