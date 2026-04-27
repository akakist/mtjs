#include "Events/System/Net/rpcEvent.h"
#include "Events/System/timerEvent.h"
#include "corelib/mutexInspector.h"
#include "Event/bcEvent.h"
#include <time.h>
#include <map>
#include "blockStreamerService.h"
#include "tools_mt.h"
#include "tree.h"
#include "events_blockStreamerService.hpp"
#include "version_mega.h"
#include "tr_exec.h"
#include "CDatabase.h"
#include "s_ed.h"
#include "QUORUM.h"
#include <SQLiteCpp/Database.h>
#include "CDatabase.h"
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include "init_root.h"



bool BlockStreamer::Service::on_startService(const systemEvent::startService*)
{
    MUTEX_INSPECTOR;

    return true;
}

bool BlockStreamer::Service::on_timer(const timerEvent::TickTimer*e)
{
    MUTEX_INSPECTOR;
    return true;
}
bool BlockStreamer::Service::on_alarm(const timerEvent::TickAlarm* e)
{
    MUTEX_INSPECTOR;
    return false;
}


bool BlockStreamer::Service::handleEvent(const REF_getter<Event::Base>& e)
{
    MUTEX_INSPECTOR;
    XTRY;
    try {
        MUTEX_INSPECTOR;
        auto& ID=e->id;
        switch(ID)
        {

        case bcEventEnum::ClientTxSubscribeREQ:
            return ClientTxSubscribeREQ(static_cast<const bcEvent::ClientTxSubscribeREQ*>(e.get()));
        case bcEventEnum::StreamBlock:
            return StreamBlock(static_cast<const bcEvent::StreamBlock*>(e.get()));

        case bcEventEnum::InvalidateRoot:
            return InvalidateRoot((const bcEvent::InvalidateRoot*)e.get());
        case bcEventEnum::ServiceInit:
            return ServiceInit((const bcEvent::ServiceInit*)e.get());
        case timerEventEnum::TickTimer:
            return on_timer((const timerEvent::TickTimer*)e.get());
        case timerEventEnum::TickAlarm:
            return on_alarm((const timerEvent::TickAlarm*)e.get());
        case systemEventEnum::startService:
            return on_startService((const systemEvent::startService*)e.get());
        case rpcEventEnum::IncomingOnAcceptor:
        {
            const rpcEvent::IncomingOnAcceptor*ev=static_cast<const rpcEvent::IncomingOnAcceptor*>(e.get());
            auto &IDA=ev->e->id;

            switch(IDA)
            {
            case bcEventEnum::ClientTxSubscribeREQ:
                return ClientTxSubscribeREQ(static_cast<const bcEvent::ClientTxSubscribeREQ*>(ev->e.get()));
            default:
                throw CommonError("unhabdled ev %d %s",IDA, iUtils->genum_name(IDA));
            }
        }
        break;
        case rpcEventEnum::IncomingOnConnector:
        {
            const rpcEvent::IncomingOnConnector*ev=static_cast<const rpcEvent::IncomingOnConnector*>(e.get());
            auto &IDC=ev->e->id;
            switch(IDC)
            {
            case bcEventEnum::ClientTxSubscribeREQ:
                return ClientTxSubscribeREQ(static_cast<const bcEvent::ClientTxSubscribeREQ*>(ev->e.get()));

            default:
                throw CommonError("unhabdled ev %d %s",IDC, iUtils->genum_name(IDC));
            }
        }
        break;

        default:
            throw CommonError("unhabdled ev %d %s",ID, iUtils->genum_name(ID));
        }



    } catch(std::exception &e)
    {
        logErr2("BlockStreamer std::exception  %s",e.what());
    }
    XPASS;
    return false;
}
#include <regex>

BlockStreamer::Service::~Service()
{
}


BlockStreamer::Service::Service(const SERVICE_id& id, const std::string& nm,IInstance* ins)
    :
    UnknownBase(nm),
    ListenerBuffered1Thread(nm,id),
    Broadcaster(ins)
{
}
bool BlockStreamer::Service::ServiceInit(const bcEvent::ServiceInit *e)
{
    conf=e;
    if(!root.valid())
        root=getRoot(conf->db.get());

    init_root(root);
    return true;
}
bool BlockStreamer::Service::InvalidateRoot(const bcEvent::InvalidateRoot*e)
{
    root=getRoot(conf->db.get());
    init_root(root);
    return true;
}

void registerBlockStreamerService(const char* pn)
{
    MUTEX_INSPECTOR;
    /// регистрация в фабрике сервиса и событий

    XTRY;
    if(pn)
    {
        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_SERVICE,ServiceEnum::BlockStreamer,"BlockStreamer",getEvents_blockStreamerService());
    }
    else
    {
        iUtils->registerService(ServiceEnum::BlockStreamer,BlockStreamer::Service::construct,"BlockStreamer");
        regEvents_blockStreamerService();
    }

    XPASS;
}




bool BlockStreamer::Service::ClientTxSubscribeREQ(const bcEvent::ClientTxSubscribeREQ* e)
{
    auto& s=clientTxSubscriptions[e->route];
    s.created_at=time(NULL);
    return true;
}
bool BlockStreamer::Service::StreamBlock(const bcEvent::StreamBlock* e)
{
    for(auto &z:clientTxSubscriptions)
    {
        passEvent(new bcEvent::ClientTxSubscribeRSP(e->payload,poppedFrontRoute(z.first)));
    }
    return true;    
}

