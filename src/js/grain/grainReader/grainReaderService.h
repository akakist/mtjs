#pragma once

#include "broadcaster.h"
#include "listenerBuffered1Thread.h"
#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/Net/httpEvent.h"
#include "Events/System/timerEvent.h"
#include "Event/bcEvent.h"
#include "DBH.h"
#include "ContractHost.h"

namespace GrainReader
{
    enum timers
    {
        TIMER_STALE_CLEANUP = 0,
    };
    enum Mode { Full = 0, Lite = 1 };

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
        bool InvalidateRoot(const bcEvent::InvalidateRoot *e);
        bool ClientMsg(const bcEvent::ClientMsg*e);
        bool on_StaleGranules(const bcEvent::StaleGranules* e);
        bool RequestIncoming(const httpEvent::RequestIncoming* e);
        bool on_LiteSetUplinkREQ(const bcEvent::LiteSetUplinkREQ* e);

        Service(const SERVICE_id&, const std::string&  nm, IInstance *ins, Mode mode);
        ~Service();

    public:
        void deinit()
        {
            ListenerBuffered1Thread::deinit();
        }

        static UnknownBase* constructFull(const SERVICE_id& id, const std::string& nm, IInstance* obj) {
            return new Service(id, nm, obj, Full);
        }
        static UnknownBase* constructLite(const SERVICE_id& id, const std::string& nm, IInstance* obj) {
            return new Service(id, nm, obj, Lite);
        }

        REF_getter<bcEvent::ServiceInit> conf=nullptr;
        REF_getter<IDatabase> db_state_3;
        REF_getter<Cellable> root = nullptr;
        REF_getter<ContractHost> contractHost = nullptr;

        Mode mode_ = Full;
        std::string uplink_http_addr_;                    // lite
        std::string db_path_;                           // lite
        std::set<msockaddr_in> http_granule_addr_;      // full

        struct StaleEntry { std::string body; int64_t ts; };
        std::map<std::string, StaleEntry> stale_cache;  // hash -> {body, ts}
        static constexpr int64_t STALE_TTL_SEC = 15 * 60;

        void cleanup_stale(int64_t now);
        bool get_stale(const std::string& hash, std::string& out);
    };
}