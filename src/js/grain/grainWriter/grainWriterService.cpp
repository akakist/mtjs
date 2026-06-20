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
#include "grainWriterService.h"
#include "event_mt.h"
#include "listenerBuffered1Thread.h"
#include "ioBuffer.h"
#include "msg.h"
#include "tools_mt.h"
#include "events_grainWriterService.hpp"
#include "unknown.h"
#include "msgFactory.h"
#include "md/md_GetUserStatusREQ.h"
#include "md/md_GetUserStatusRSP.h"
#include "init_root.h"

bool GrainWriter::Service::on_startService(const systemEvent::startService *)
{
    MUTEX_INSPECTOR;
    sendEvent(ServiceEnum::Timer, new timerEvent::SetTimer(1,NULL,NULL,1., this));

    return true;
}

bool GrainWriter::Service::on_timer(const timerEvent::TickTimer *e)
{
    MUTEX_INSPECTOR;
    // logErr2("tid %d",e->tid);
    if(e->tid==1)
    {
        if(db_to_save.cells.size())
        {
            db_state->write_batch(db_to_save);
            logErr2("written %d granules",db_to_save.cells.size());
            db_to_save.clear();
        }
    }
    return true;
}
bool GrainWriter::Service::on_alarm(const timerEvent::TickAlarm *e)
{
    MUTEX_INSPECTOR;
    logErr2("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! CREATE SNAPSHOT");
    db_state->create_snapshot();
    return true;
}

bool GrainWriter::Service::handleEvent(const REF_getter<Event::Base> &e)
{
    MUTEX_INSPECTOR;
    XTRY;
    try
    {
        MUTEX_INSPECTOR;
        auto &ID = e->id;
        switch (ID)
        {
        case bcEventEnum::WriteGranules:
            return WriteGranules((const bcEvent::WriteGranules *)e.get());
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
        logErr2("GrainWriter std::exception  %s", e.what());
    }
    XPASS;
    return false;
}


GrainWriter::Service::~Service()
{
}

GrainWriter::Service::Service(const SERVICE_id &id, const std::string &nm, IInstance *ins)
    : UnknownBase(nm),
      ListenerBuffered1Thread(nm, id),
      Broadcaster(ins)
{
    snapshot_modulus=ins->getConfig()->get_int64_t("snapshot_modulus",10000,"every this modulus of epoch make snapshot");
}
bool GrainWriter::Service::ServiceInit(const bcEvent::ServiceInit *e)
{
    conf = e;
    root=e->root;
    // if (!root.valid())
    //     root = getRoot(conf->db.get());

    // init_root(root);
    return true;
}
bool GrainWriter::Service::InvalidateRoot(const bcEvent::InvalidateRoot *e)
{
    root=e->root;
    // root = getRoot(conf->db.get());
    // init_root(root);
    return true;
}
bool GrainWriter::Service::ClientMsg(const bcEvent::ClientMsg *e)
{
    throw CommonError("unhandled GrainWriter ClientMsg %s", e->msg.c_str());
#ifdef KALL
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
        rsp->balance=u->getBalance();
        rsp->nonce=u->getNonce();
        passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));


    }
    break;
    }
#endif
    return true;
}
bool GrainWriter::Service::WriteGranules(const bcEvent::WriteGranules *e)
{
    for(auto& z: e->gs.cells)
    {
        db_to_save.add(z.first,z.second);
    }
    db_state=e->db;
    if(e->epoch % snapshot_modulus == 0)
        sendEvent(ServiceEnum::Timer, new timerEvent::SetAlarm(1,NULL,NULL,1.,this));

    return true;
}

void registerGrainWriterService(const char *pn)
{
    MUTEX_INSPECTOR;
    /// регистрация в фабрике сервиса и событий

    XTRY;
    if (pn)
    {
        iUtils->registerPlugingInfo(pn, IUtils::PLUGIN_TYPE_SERVICE, ServiceEnum::GrainWriter, "GrainWriter", getEvents_grainWriterService());
    }
    else
    {
        iUtils->registerService(ServiceEnum::GrainWriter, GrainWriter::Service::construct, "GrainWriter");
        regEvents_grainWriterService();
    }

    XPASS;
}
