#include "Events/System/Net/rpcEvent.h"
#include "Events/System/timerEvent.h"
#include "corelib/mutexInspector.h"
#include "Event/bcEvent.h"
#include <time.h>
#include <map>
#include "grainReaderService.h"
#include "tools_mt.h"
#include "tree.h"
#include "events_grainReaderService.hpp"
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



bool GrainReader::Service::on_startService(const systemEvent::startService*)
{
    MUTEX_INSPECTOR;

    return true;
}

bool GrainReader::Service::on_timer(const timerEvent::TickTimer*e)
{
    MUTEX_INSPECTOR;
    return true;
}
bool GrainReader::Service::on_alarm(const timerEvent::TickAlarm* e)
{
    MUTEX_INSPECTOR;
    return false;
}


bool GrainReader::Service::handleEvent(const REF_getter<Event::Base>& e)
{
    MUTEX_INSPECTOR;
    XTRY;
    try {
        MUTEX_INSPECTOR;
        auto& ID=e->id;
        switch(ID)
        {
        case bcEventEnum::InvalidateRoot:
            return InvalidateRoot((const bcEvent::InvalidateRoot*)e.get());
        case bcEventEnum::ClientMsg:
            return ClientMsg((const bcEvent::ClientMsg*)e.get());
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
            case bcEventEnum::ClientMsg:
                return ClientMsg((const bcEvent::ClientMsg*)ev->e.get());
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
            case bcEventEnum::ClientMsg:
                return ClientMsg((const bcEvent::ClientMsg*)ev->e.get());

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
        logErr2("GrainReader std::exception  %s",e.what());
    }
    XPASS;
    return false;
}
#include <regex>

GrainReader::Service::~Service()
{
}


GrainReader::Service::Service(const SERVICE_id& id, const std::string& nm,IInstance* ins)
    :
    UnknownBase(nm),
    ListenerBuffered1Thread(nm,id),
    Broadcaster(ins)
{
}
bool GrainReader::Service::ServiceInit(const bcEvent::ServiceInit *e)
{
    conf=e;
    if(!root.valid())
        root=getRoot(conf->db.get());

    init_root(root);
    return true;
}
bool GrainReader::Service::InvalidateRoot(const bcEvent::InvalidateRoot*e)
{
    root=getRoot(conf->db.get());
    init_root(root);
    return true;
}
bool GrainReader::Service::ClientMsg(const bcEvent::ClientMsg*e)
{


    MUTEX_INSPECTOR;
    inBuffer in(e->msg);

    auto p=in.get_PN();
    THASH_id hash;
    hash=blake2b_hash(e->msg);
    switch(p)
    {
    case msgid::user_request:
    {
        msg::user_request ur(in);


        inBuffer in2(ur.payload);
        auto p2=in2.get_PN();
        switch(p2)
        {
        case msgid::get_user_status_req:
        {
            MUTEX_INSPECTOR;
            msg::get_user_status_req rq(in2);
            std::optional<std::string> err;
            auto u=root->getUser(rq.address_pk_ed,NULL);
            if(!u.valid())
                err="user not found "+base62::encode(rq.address_pk_ed);

            msg::get_user_status_rsp r;
            if(!err)
            {
                r.address_pk_ed=rq.address_pk_ed;
                r.nonce=u->nonce;
                r.balance=u->balance;
            }
            else
            {
                r.address_pk_ed=rq.address_pk_ed;
                r.nonce=0;
                r.balance=0;

            }
            if(err)
                logErr2("get_user_status_req error %s",err->c_str());
            passEvent(new bcEvent::ClientMsgReply(hash, r.getBuffer(),poppedFrontRoute(e->route)));

        }
        break;
        default:
            throw CommonError("unhandled msgid sdf %d",p);
        }


    }
    break;
    break;
    default:
        throw CommonError("unhandled msgid e. ff %d",p);
    }

    return true;
}


void registerGrainReaderService(const char* pn)
{
    MUTEX_INSPECTOR;
    /// регистрация в фабрике сервиса и событий

    XTRY;
    if(pn)
    {
        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_SERVICE,ServiceEnum::GrainReader,"GrainReader",getEvents_grainReaderService());
    }
    else
    {
        iUtils->registerService(ServiceEnum::GrainReader,GrainReader::Service::construct,"GrainReader");
        regEvents_grainReaderService();
    }

    XPASS;
}




