#include "Events/System/Net/rpcEvent.h"
#include "Events/System/timerEvent.h"
#include "corelib/mutexInspector.h"
#include "Event/bcEvent.h"
#include <time.h>
#include <map>
#include "blockValidatorService.h"
#include "tools_mt.h"
#include "tree.h"
#include "events_blockValidatorService.hpp"
#include "version_mega.h"
#include "tr_exec.h"
#include "CDatabase.h"
#include "s_ed.h"
#include "QUORUM.h"
#include <SQLiteCpp/Database.h>
#include "CDatabase.h"

bool BlockValidator::Service::on_startService(const systemEvent::startService*)
{
    MUTEX_INSPECTOR;


    return true;
}

bool BlockValidator::Service::on_timer(const timerEvent::TickTimer*e)
{
    MUTEX_INSPECTOR;
    return true;
}
bool BlockValidator::Service::on_alarm(const timerEvent::TickAlarm* e)
{
    MUTEX_INSPECTOR;
    return false;
}


bool BlockValidator::Service::handleEvent(const REF_getter<Event::Base>& e)
{
    MUTEX_INSPECTOR;
    XTRY;
    try {
        MUTEX_INSPECTOR;
        auto& ID=e->id;
        switch(ID)
        {
        case timerEventEnum::TickTimer:
            return on_timer((const timerEvent::TickTimer*)e.get());
        case timerEventEnum::TickAlarm:
            return on_alarm((const timerEvent::TickAlarm*)e.get());
        case webHandlerEventEnum::RequestIncoming:
            return on_RequestIncoming((const webHandlerEvent::RequestIncoming*)e.get());
        case systemEventEnum::startService:
            return on_startService((const systemEvent::startService*)e.get());
        case bcEventEnum::ServiceInit:
            return ServiceInit((const bcEvent::ServiceInit*)e.get());
        case rpcEventEnum::IncomingOnAcceptor:
        {
            const rpcEvent::IncomingOnAcceptor*ev=static_cast<const rpcEvent::IncomingOnAcceptor*>(e.get());
            auto &IDA=ev->e->id;

            switch(IDA)
            {
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
        logErr2("Node std::exception  %s",e.what());
    }
    XPASS;
    return false;
}

BlockValidator::Service::~Service()
{
}


BlockValidator::Service::Service(const SERVICE_id& id, const std::string& nm,IInstance* ins)
    :
    UnknownBase(nm),
    ListenerBuffered1Thread(nm,id),
    Broadcaster(ins)
{
}

bool BlockValidator::Service::on_RequestIncoming(const webHandlerEvent::RequestIncoming* )
{
    return true;
}
void registerBlockValidatorService(const char* pn)
{
    MUTEX_INSPECTOR;

    XTRY;
    if(pn)
    {
        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_SERVICE,ServiceEnum::BlockValidator,"BlockValidator",getEvents_blockValidatorService());
    }
    else
    {
        iUtils->registerService(ServiceEnum::BlockValidator,BlockValidator::Service::construct,"BlockValidator");
        regEvents_blockValidatorService();
    }
    XPASS;
}



bool BlockValidator::Service::ServiceInit(const bcEvent::ServiceInit* e)
{
    conf=e;
    return true;    
}


bool BlockValidator::Service::RequestIncoming(const httpEvent::RequestIncoming* e)
{

    return true;

}
