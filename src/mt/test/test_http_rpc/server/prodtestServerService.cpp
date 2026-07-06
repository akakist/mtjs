#include "corelib/mutexInspector.h"
#include <time.h>
#include <time.h>

#include "tools_mt.h"

#include <map>
#include "prodtestServerService.h"
#include "Events/Ping.h"
#include "Events/System/timerEvent.h"
#include "Events/System/Net/rpcEvent.h"
#include "events_prodtestServerService.hpp"

bool prodtestServer::Service::on_startService(const systemEvent::startService*)
{
    MUTEX_INSPECTOR;
    try {

    }  catch (CommonError& e) {

        logErr2("catch %s",e.what());
    }

    return true;
}


bool prodtestServer::Service::handleEvent(const REF_getter<Event::Base>& e)
{
    XTRY;
    try {
        MUTEX_INSPECTOR;
        auto& ID=e->id;
        if(timerEventEnum::TickTimer==ID)
        {
            const timerEvent::TickTimer*ev=static_cast<const timerEvent::TickTimer*>(e.get());
            return true;
        }
        if(systemEventEnum::startService==ID)
            return on_startService((const systemEvent::startService*)e.get());

        if(prodtestEventEnum::AddTaskREQ==ID)
            return on_AddTaskREQ((const prodtestEvent::AddTaskREQ*)e.get());


        if(rpcEventEnum::IncomingOnAcceptor==ID)
        {
            auto E=(rpcEvent::IncomingOnAcceptor*)e.get();
            auto &IDA=E->e->id;
            if(prodtestEventEnum::AddTaskREQ==IDA)
                return on_AddTaskREQ((const prodtestEvent::AddTaskREQ*)E->e.get());
        }

    } catch(const std::exception &e)
    {
        logErr2(" std::exception  %s",e.what());
    }
    XPASS;
    return false;
}



prodtestServer::Service::~Service()
{
}


prodtestServer::Service::Service(const SERVICE_id& id, const std::string& nm,IInstance* ins):
    UnknownBase(nm),
    ListenerBuffered1Thread(nm,id),
    Broadcaster(ins)
{
}

void registerprodtestServerService(const char* pn)
{
    MUTEX_INSPECTOR;

    XTRY;
    if(pn)
    {
        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_SERVICE,ServiceEnum::prodtestServer,"prodtestServer",getEvents_prodtestServerService());
    }
    else
    {
        iUtils->registerService(ServiceEnum::prodtestServer,prodtestServer::Service::construct,"prodtestServer");
        regEvents_prodtestServerService();
    }
    XPASS;
}

int cnt=0;


bool prodtestServer::Service::on_AddTaskREQ(const prodtestEvent::AddTaskREQ* e)
{
    cnt++;
    std::string xored=e->sampleString+"123";


    passEvent(new prodtestEvent::AddTaskRSP(e->session,xored,e->count,poppedFrontRoute(e->route)));
    return true;
}

