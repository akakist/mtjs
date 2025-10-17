#include "corelib/mutexInspector.h"
#include <time.h>
#include <time.h>
#include <map>
#include "testWebServerService.h"
#include "___testEvent.h"
#include "Events/System/Net/httpEvent.h"
#include "Events/System/Net/rpcEvent.h"
#include "version_mega.h"



bool testWebServer::Service::on_startService(const systemEvent::startService*)
{
    MUTEX_INSPECTOR;


    auto svs=dynamic_cast<ListenerBase*> (iInstance->getServiceOrCreate(ServiceEnum::HTTP));
    if(!svs)
        throw CommonError("if(!svs)");
        SECURE sec;
        sec.use_ssl=false;
    sendEvent(svs,new httpEvent::DoListen(bindAddr,sec,this));

    return true;
}


bool testWebServer::Service::handleEvent(const REF_getter<Event::Base>& e)
{
    XTRY;
    try {
        MUTEX_INSPECTOR;
        auto& ID=e->id;
        if(httpEventEnum::RequestIncoming==ID)
            return on_RequestIncoming((const httpEvent::RequestIncoming*)e.get());
        if(systemEventEnum::startService==ID)
            return on_startService((const systemEvent::startService*)e.get());



        if(rpcEventEnum::IncomingOnConnector==ID)
        {
            rpcEvent::IncomingOnConnector *E=(rpcEvent::IncomingOnConnector *) e.get();
            auto& IDC=E->e->id;


            return false;
        }

        if(rpcEventEnum::IncomingOnAcceptor==ID)
        {
            rpcEvent::IncomingOnAcceptor *E=(rpcEvent::IncomingOnAcceptor *) e.get();
            auto& IDA=E->e->id;


            return false;
        }


    }
    catch(const CommonError& e)
    {
        logErr2("testWebServer CommonError  %s",e.what());

    }
    catch(const std::exception &e)
    {
        logErr2("testWebServer std::exception  %s",e.what());
    }
    XPASS;
    return false;
}

testWebServer::Service::~Service()
{
}


testWebServer::Service::Service(const SERVICE_id& id, const std::string& nm,IInstance* ins):
    UnknownBase(nm),
    ListenerSimple(nm,id),
    Broadcaster(ins), iInstance(ins)
{
    auto ba=ins->getConfig()->get_tcpaddr("bindAddr","0.0.0.0:8088","http listen address");
    if(ba.size()==0)
        throw CommonError("if(ba.size()==0)");

    bindAddr=*ba.begin();

}

void registertestServerWebService(const char* pn)
{
    MUTEX_INSPECTOR;

    XTRY;
    if(pn)
    {
        std::set<EVENT_id> es;
        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_SERVICE,ServiceEnum::testWebServer,"testHTTP",es);
    }
    else
    {
        iUtils->registerService(ServiceEnum::testWebServer,testWebServer::Service::construct,"testHTTP");
//        regEvents_prodtestWebServer();
    }
    XPASS;
}







bool testWebServer::Service::on_RequestIncoming(const httpEvent::RequestIncoming*e)
{

    HTTP::Response resp(e->req);
    resp.make_response("<div>received response </>");



    return true;
}


