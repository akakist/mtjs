#include "Events/System/Net/rpcEvent.h"
#include "Events/System/Run/startServiceEvent.h"
#include "IUtils.h"
#include "Events/System/timerEvent.h"
#include "REF.h"
#include "IUtils.h"
#include "commonError.h"
#include "SERVICE_id.h"
#include "IInstance.h"
#include "broadcaster.h"
#include "corelib/mutexInspector.h"
#include "Event/bcEvent.h"
#include <exception>
#include <string>
#include <time.h>
#include "blockStreamerService.h"
#include "event_mt.h"
#include "listenerBuffered1Thread.h"
#include "events_blockStreamerService.hpp"
#include "unknown.h"
#include "tools_mt.h"

bool BlockStreamer::Service::on_startService(const systemEvent::startService *)
{
    MUTEX_INSPECTOR;
    sendEvent(ServiceEnum::Timer, new timerEvent::SetTimer(TIMER_CLEAN_SUBSCRIPTIONS, NULL, NULL, 10., this));
    return true;
}

bool BlockStreamer::Service::on_timer(const timerEvent::TickTimer *e)
{
    MUTEX_INSPECTOR;
    if (e->tid == TIMER_CLEAN_SUBSCRIPTIONS) {
        time_t now = time(NULL);
        for (auto it = allBlocksSubscriptions.begin(); it != allBlocksSubscriptions.end(); ) {
            if (now - (*it)->created_at > (*it)->ttl) it = allBlocksSubscriptions.erase(it);
            else ++it;
        }
        for (auto it = txWaitSubscriptions.begin(); it != txWaitSubscriptions.end(); ) {
            auto &v = it->second;
            for (auto vit = v.begin(); vit != v.end(); ) {
                if (now - (*vit)->created_at > (*vit)->ttl) vit = v.erase(vit);
                else ++vit;
            }
            if (v.empty()) it = txWaitSubscriptions.erase(it);
            else ++it;
        }
    }
    return true;
}

bool BlockStreamer::Service::on_alarm(const timerEvent::TickAlarm *e)
{
    MUTEX_INSPECTOR;
    return false;
}

bool BlockStreamer::Service::handleEvent(const REF_getter<Event::Base> &e)
{
    MUTEX_INSPECTOR;
    XTRY;
    try
    {
        MUTEX_INSPECTOR;
        auto &ID = e->id;
        switch (ID)
        {

        case bcEventEnum::ClientTxSubscribeREQ:
            return ClientTxSubscribeREQ(static_cast<const bcEvent::ClientTxSubscribeREQ *>(e.get()));
        case bcEventEnum::StreamBlock:
            return StreamBlock(static_cast<const bcEvent::StreamBlock *>(e.get()));

        case bcEventEnum::ServiceInit:
            return ServiceInit((const bcEvent::ServiceInit *)e.get());
        case timerEventEnum::TickTimer:
            return on_timer((const timerEvent::TickTimer *)e.get());
        case timerEventEnum::TickAlarm:
            return on_alarm((const timerEvent::TickAlarm *)e.get());
        case systemEventEnum::startService:
            return on_startService((const systemEvent::startService *)e.get());
        case rpcEventEnum::IncomingOnAcceptor:
        {
            const rpcEvent::IncomingOnAcceptor *ev = static_cast<const rpcEvent::IncomingOnAcceptor *>(e.get());
            auto &IDA = ev->e->id;

            switch (IDA)
            {
                case bcEventEnum::ClientTxWaitREQ:
                    return ClientTxWaitREQ(static_cast<const bcEvent::ClientTxWaitREQ *>(ev->e.get()));
                case bcEventEnum::ClientTxUnsubscribeREQ:
                    return ClientTxUnsubscribeREQ(static_cast<const bcEvent::ClientTxUnsubscribeREQ *>(ev->e.get()));
                case bcEventEnum::ClientTxSubscribeREQ:
                    return ClientTxSubscribeREQ(static_cast<const bcEvent::ClientTxSubscribeREQ *>(ev->e.get()));
            default:
                throw CommonError("unhabdled ev %d %s", IDA, iUtils->genum_name(IDA));
            }
        }
        break;
        case rpcEventEnum::IncomingOnConnector:
        {
            const rpcEvent::IncomingOnConnector *ev = static_cast<const rpcEvent::IncomingOnConnector *>(e.get());
            auto &IDC = ev->e->id;
            switch (IDC)
            {
            case bcEventEnum::ClientTxWaitREQ:
                return ClientTxWaitREQ(static_cast<const bcEvent::ClientTxWaitREQ *>(ev->e.get()));
            case bcEventEnum::ClientTxUnsubscribeREQ:
                return ClientTxUnsubscribeREQ(static_cast<const bcEvent::ClientTxUnsubscribeREQ *>(ev->e.get()));
            case bcEventEnum::ClientTxSubscribeREQ:
                return ClientTxSubscribeREQ(static_cast<const bcEvent::ClientTxSubscribeREQ *>(ev->e.get()));

            default:
                throw CommonError("unhabdled ev %d %s", IDC, iUtils->genum_name(IDC));
            }
        }
        break;

        default:
            throw CommonError("unhabdled ev %d %s", ID, iUtils->genum_name(ID));
        }
    }
    catch (std::exception &e)
    {
        logErr2("BlockStreamer std::exception  %s", e.what());
    }
    XPASS;
    return false;
}

BlockStreamer::Service::~Service()
{
}

BlockStreamer::Service::Service(const SERVICE_id &id, const std::string &nm, IInstance *ins)
    : UnknownBase(nm),
      ListenerBuffered1Thread(nm, id),
      Broadcaster(ins)
{
}
bool BlockStreamer::Service::ServiceInit(const bcEvent::ServiceInit *e)
{
    return true;
}

void registerBlockStreamerService(const char *pn)
{
    MUTEX_INSPECTOR;
    /// регистрация в фабрике сервиса и событий

    XTRY;
    if (pn)
    {
        iUtils->registerPlugingInfo(pn, IUtils::PLUGIN_TYPE_SERVICE, ServiceEnum::BlockStreamer, "BlockStreamer", getEvents_blockStreamerService());
    }
    else
    {
        iUtils->registerService(ServiceEnum::BlockStreamer, BlockStreamer::Service::construct, "BlockStreamer");
        regEvents_blockStreamerService();
    }

    XPASS;
}

bool BlockStreamer::Service::ClientTxSubscribeREQ(const bcEvent::ClientTxSubscribeREQ *e)
{
    // auto &s = clientTxSubscriptions[e->route];
    allBlocksSubscriptions.push_back(new clientTxSubscription(e->route, 1800));
    // s.created_at = time(NULL);
    return true;
}
bool BlockStreamer::Service::ClientTxWaitREQ(const bcEvent::ClientTxWaitREQ *e)
{
    time_t ttl = e->ttl > 0 ? e->ttl : 60;
    // logErr2("[BlockStreamer] ClientTxWaitREQ hash=%s ttl=%ld, txWaitSubscriptions.size=%d",
    //         e->tx_hash.str().c_str(), (long)ttl, (int)txWaitSubscriptions.size());
    // auto hex = base16::encode(e->tx_hash.container);
    txWaitSubscriptions[e->tx_hash].push_back(new txWaitSubscription(e->route, ttl));
    return true;
}

bool BlockStreamer::Service::ClientTxUnsubscribeREQ(const bcEvent::ClientTxUnsubscribeREQ *e)
{
    if (e->tx_hash.container.empty()) {
        for (auto it = allBlocksSubscriptions.begin(); it != allBlocksSubscriptions.end(); ) {
            if ((*it)->backRoute == e->route) it = allBlocksSubscriptions.erase(it);
            else ++it;
        }
    } else {
        auto it = txWaitSubscriptions.find(e->tx_hash);
        if (it != txWaitSubscriptions.end()) {
            auto &v = it->second;
            for (auto vit = v.begin(); vit != v.end(); ) {
                if ((*vit)->backRoute == e->route) vit = v.erase(vit);
                else ++vit;
            }
            if (v.empty()) txWaitSubscriptions.erase(it);
        }
    }
    return true;
}

bool BlockStreamer::Service::StreamBlock(const bcEvent::StreamBlock *e)
{
//   logErr2("[BlockStreamer] StreamBlock tx_hashes=%d allBlocks=%d txWait=%d",
//             (int)e->tx_hashes.size(), (int)allBlocksSubscriptions.size(), (int)txWaitSubscriptions.size());
   // 1) «все блоки»
    for (auto &sub : allBlocksSubscriptions) {
        passEvent(new bcEvent::ClientTxSubscribeRSP(e->blockAccepted2REQ, e->tx_hashes,
            e->diffs, e->att_data, poppedFrontRoute(sub->backRoute)));
    }
    // 2) «одна TX» — по бинарному tx_hash.container
    for (auto &h : e->tx_hashes) {
        auto it = txWaitSubscriptions.find(h);
        if (it == txWaitSubscriptions.end()) continue;
        // logErr2("[BlockStreamer] StreamBlock find %s -> %s",
        //         h.str().c_str(), it != txWaitSubscriptions.end() ? "found" : "not found");
        for (auto &sub : it->second) {
            passEvent(new bcEvent::ClientTxSubscribeRSP(e->blockAccepted2REQ, e->tx_hashes,
                e->diffs, e->att_data, poppedFrontRoute(sub->backRoute)));
        }
        txWaitSubscriptions.erase(it);
    }
    return true;
}