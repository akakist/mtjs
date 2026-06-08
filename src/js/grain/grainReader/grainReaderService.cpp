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
#include "grainReaderService.h"
#include "event_mt.h"
#include "listenerBuffered1Thread.h"
#include "ioBuffer.h"
#include "msg.h"
#include "tools_mt.h"
#include "events_grainReaderService.hpp"
#include "unknown.h"
#include "msgFactory.h"
#include "md/md_GetUserStatusREQ.h"
#include "md/md_GetUserStatusRSP.h"

bool GrainReader::Service::on_startService(const systemEvent::startService *)
{
    MUTEX_INSPECTOR;

    return true;
}

bool GrainReader::Service::on_timer(const timerEvent::TickTimer *e)
{
    MUTEX_INSPECTOR;
    return true;
}
bool GrainReader::Service::on_alarm(const timerEvent::TickAlarm *e)
{
    MUTEX_INSPECTOR;
    return false;
}

bool GrainReader::Service::handleEvent(const REF_getter<Event::Base> &e)
{
    MUTEX_INSPECTOR;
    XTRY;
    try
    {
        MUTEX_INSPECTOR;
        auto &ID = e->id;
        switch (ID)
        {
        case bcEventEnum::InvalidateRoot:
            return InvalidateRoot((const bcEvent::InvalidateRoot *)e.get());
        case bcEventEnum::ClientMsg:
            return ClientMsg((const bcEvent::ClientMsg *)e.get());
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
            case bcEventEnum::ClientMsg:
                return ClientMsg((const bcEvent::ClientMsg *)ev->e.get());
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
            case bcEventEnum::ClientMsg:
                return ClientMsg((const bcEvent::ClientMsg *)ev->e.get());

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
        logErr2("GrainReader std::exception  %s", e.what());
    }
    XPASS;
    return false;
}


GrainReader::Service::~Service()
{
}

GrainReader::Service::Service(const SERVICE_id &id, const std::string &nm, IInstance *ins)
    : UnknownBase(nm),
      ListenerBuffered1Thread(nm, id),
      Broadcaster(ins)
{
}
bool GrainReader::Service::ServiceInit(const bcEvent::ServiceInit *e)
{
    conf = e;
    if (!root.valid())
        root = getRoot(conf->db.get());

    init_root(root);
    return true;
}
bool GrainReader::Service::InvalidateRoot(const bcEvent::InvalidateRoot *e)
{
    root = getRoot(conf->db.get());
    init_root(root);
    return true;
}
bool GrainReader::Service::ClientMsg(const bcEvent::ClientMsg *e)
{

    MUTEX_INSPECTOR;
    inBuffer in(e->msg);

    auto p = in.get_PN();
    REF_getter<MsgData::Base> b=msgFactory.create(p);
    b->unpack(in);
    auto hash=b->getHash();

    switch(p)
    {
    case msgid::GetUserStatusREQ:
    {
        auto pp=(MsgData::GetUserStatusREQ*) b.get();
        auto u = root->getUserState(pp->user_pk_bin_ed);

        REF_getter<MsgData::GetUserStatusRSP> rsp=new MsgData::GetUserStatusRSP;
        rsp->balance=u->balance;
        rsp->nonce=u->nonce;
        passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));


    }
    break;
    }

    return true;
}

void registerGrainReaderService(const char *pn)
{
    MUTEX_INSPECTOR;
    /// регистрация в фабрике сервиса и событий

    XTRY;
    if (pn)
    {
        iUtils->registerPlugingInfo(pn, IUtils::PLUGIN_TYPE_SERVICE, ServiceEnum::GrainReader, "GrainReader", getEvents_grainReaderService());
    }
    else
    {
        iUtils->registerService(ServiceEnum::GrainReader, GrainReader::Service::construct, "GrainReader");
        regEvents_grainReaderService();
    }

    XPASS;
}
