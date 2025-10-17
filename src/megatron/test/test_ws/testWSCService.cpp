#include "corelib/mutexInspector.h"
#include <time.h>
#include <time.h>
#include <map>
#include "testWSCService.h"
#include "___testEvent2.h"
#include "Events/System/Net/httpEvent.h"
#include "Events/System/Net/webSocketClientEvent.h"
#include "Events/System/Net/rpcEvent.h"
#include "version_mega.h"



bool testWSC::Service::on_startService(const systemEvent::startService*)
{
    MUTEX_INSPECTOR;
    auto sid=iUtils->getNewSocketId();
    sendEvent(ServiceEnum::WebSocketClient,new webSocketClientEvent::Connect(sid,url,"wstest",this));
    return true;
}


bool testWSC::Service::handleEvent(const REF_getter<Event::Base>& e)
{
    XTRY;
    try {
        MUTEX_INSPECTOR;
        auto& ID=e->id;
        if(httpEventEnum::RequestIncoming==ID)
            return on_RequestIncoming((const httpEvent::RequestIncoming*)e.get());
        if(systemEventEnum::startService==ID)
            return on_startService((const systemEvent::startService*)e.get());

        if(webSocketClientEventEnum::ConnectFailed==ID)
            return ConnectFailed((const webSocketClientEvent::ConnectFailed*)e.get());
        if(webSocketClientEventEnum::Connected==ID)
            return Connected((const webSocketClientEvent::Connected*)e.get());
        if(webSocketClientEventEnum::Disconnected==ID)
            return Disconnected((const webSocketClientEvent::Disconnected*)e.get());
    
        if(webSocketClientEventEnum::Received==ID)
            return Received((const webSocketClientEvent::Received*)e.get());



    }
    catch(const CommonError& e)
    {
        logErr2("testWSC CommonError  %s",e.what());

    }
    catch(const std::exception &e)
    {
        logErr2("testWSC std::exception  %s",e.what());
    }
    XPASS;
    return false;
}

testWSC::Service::~Service()
{
}


testWSC::Service::Service(const SERVICE_id& id, const std::string& nm,IInstance* ins):
    UnknownBase(nm),
    ListenerSimple(nm,id),
    Broadcaster(ins), iInstance(ins)
{
    // auto ba=ins->getConfig()->get_tcpaddr("bindAddr","0.0.0.0:8088","http listen address");
    // if(ba.size()==0)
    //     throw CommonError("if(ba.size()==0)");

    // bindAddr=*ba.begin();
    url=ins->getConfig()->get_string("url","ws://localhost/zaza","");

}

void registerTestWSCService(const char* pn)
{
    MUTEX_INSPECTOR;

    XTRY;
    if(pn)
    {
        std::set<EVENT_id> es;
        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_SERVICE,ServiceEnum::testWSC,"testWSC",es);
    }
    else
    {
        iUtils->registerService(ServiceEnum::testWSC,testWSC::Service::construct,"testWSC");
//        regEvents_prodtestWebServer();
    }
    XPASS;
}







bool testWSC::Service::on_RequestIncoming(const httpEvent::RequestIncoming*e)
{

    HTTP::Response resp(e->req);
    {
        resp.make_response("<div>received response </>");


    }

    return true;
}

bool testWSC::Service::Connected(const webSocketClientEvent::Connected*e)
{
    sendEvent(ServiceEnum::WebSocketClient,new webSocketClientEvent::Send(e->esi,"hello wasya",this));
    return true;
}
bool testWSC::Service::ConnectFailed(const webSocketClientEvent::ConnectFailed*)
{
    return true;

}
bool testWSC::Service::Disconnected(const webSocketClientEvent::Disconnected*)
{
    return true;

}
static int n=0;
bool testWSC::Service::Received(const webSocketClientEvent::Received* e)
{
    std::string msg= "new req "+std::to_string(n++);
    sendEvent(ServiceEnum::WebSocketClient,new webSocketClientEvent::Send(e->esi,msg,this));
    return true;

}



