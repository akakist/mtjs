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
#include <map>
#include "blockStreamerService.h"
#include "event_mt.h"
#include "listenerBuffered1Thread.h"
#include "tools_mt.h"
#include "events_blockStreamerService.hpp"
#include "unknown.h"

bool BlockStreamer::Service::on_startService(const systemEvent::startService *)
{
    MUTEX_INSPECTOR;

    return true;
}

bool BlockStreamer::Service::on_timer(const timerEvent::TickTimer *e)
{
    MUTEX_INSPECTOR;
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

        case bcEventEnum::InvalidateRoot:
            return InvalidateRoot((const bcEvent::InvalidateRoot *)e.get());
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
bool BlockStreamer::Service::InvalidateRoot(const bcEvent::InvalidateRoot *e)
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
    auto &s = clientTxSubscriptions[e->route];
    s.created_at = time(NULL);
    return true;
}
bool BlockStreamer::Service::StreamBlock(const bcEvent::StreamBlock *e)
{
    for (auto &z : clientTxSubscriptions)
    {
        passEvent(new bcEvent::ClientTxSubscribeRSP(e->blockStore, e->att_data, poppedFrontRoute(z.first)));
    }
    return true;
}
