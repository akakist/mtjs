#include "errorDispatcherService.h"
#include <stdarg.h>

#include "tools_mt.h"
#include "events_errorDispatcher.hpp"
ErrorDispatcher::Service::Service(const SERVICE_id& id, const std::string& nm, IInstance *ifa):
    UnknownBase(nm),
    ListenerBuffered1Thread(nm,id),Broadcaster(ifa)

{


}

ErrorDispatcher::Service::~Service()
{
}
bool ErrorDispatcher::Service::on_startService(const systemEvent::startService* e)
{
    if(!e) throw CommonError("!e");
    return true;
}

void registerErrorDispatcherService(const char* pn)
{
    XTRY;
    if(pn)
    {

        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_SERVICE,ServiceEnum::ErrorDispatcher,"ErrorDispatcher",getEvents_errorDispatcher());
    }
    else
    {
        iUtils->registerService(ServiceEnum::ErrorDispatcher,ErrorDispatcher::Service::construct,"ErrorDispatcher");
        regEvents_errorDispatcher();
    }
    XPASS;
}

bool ErrorDispatcher::Service::on_errorDispatcherSendMessage(const errorDispatcherEvent::SendMessage* e)
{
    MUTEX_INSPECTOR;
    XTRY;
    if(!e) throw CommonError("!e");
    for(auto& i:m_subscribers)
    {
        {
            passEvent(new errorDispatcherEvent::NotifySubscriber(e->opcode,e->msg,poppedFrontRoute(i)));
        }
    }
    XPASS;
    return true;
}

bool ErrorDispatcher::Service::on_IncomingOnAcceptor(const rpcEvent::IncomingOnAcceptor*ev)
{
    MUTEX_INSPECTOR;
    XTRY;
    if(!ev) throw CommonError("!ev");
    auto &IDA=ev->e->id;
    if( errorDispatcherEventEnum::Subscribe==IDA)
        return on_errorDispatcherSubscribe(static_cast<const errorDispatcherEvent::Subscribe* > (ev->e.get()));
    if( errorDispatcherEventEnum::Unsubscribe==IDA)
        return on_errorDispatcherUnsubscribeAll(static_cast<const errorDispatcherEvent::Unsubscribe* > (ev->e.get()));
    if( errorDispatcherEventEnum::SendMessage==IDA)
        return on_errorDispatcherSendMessage(static_cast<const errorDispatcherEvent::SendMessage* > (ev->e.get()));

    logErr2("ErrorDispatcher: unhandled event %s %s %d",iUtils->genum_name(ev->id),__FILE__,__LINE__);
    XPASS;
    return false;
}
bool ErrorDispatcher::Service::on_errorDispatcherSubscribe(const errorDispatcherEvent::Subscribe* e)
{
    MUTEX_INSPECTOR;
    XTRY;
    if(!e) throw CommonError("!e");
    m_subscribers.insert(e->route);
    XPASS;
    return true;
}
bool ErrorDispatcher::Service::on_errorDispatcherUnsubscribeAll(const errorDispatcherEvent::Unsubscribe* e)
{
    MUTEX_INSPECTOR;
    XTRY;
    if(!e) throw CommonError("!e");
    m_subscribers.erase(e->route);
    XPASS;
    return true;
}
bool ErrorDispatcher::Service::handleEvent(const REF_getter<Event::Base>& e)
{
    MUTEX_INSPECTOR;
    XTRY;
    auto &ID=e->id;

    auto z=static_cast<const errorDispatcherEvent::Subscribe*>(e.get());
    if(z)
        return on_errorDispatcherSubscribe(z);

    if( errorDispatcherEventEnum::Subscribe==ID)
        return on_errorDispatcherSubscribe(static_cast<const errorDispatcherEvent::Subscribe* > (e.get()));
    if( errorDispatcherEventEnum::Unsubscribe==ID)
        return on_errorDispatcherUnsubscribeAll(static_cast<const errorDispatcherEvent::Unsubscribe* > (e.get()));
    if( errorDispatcherEventEnum::SendMessage==ID)
        return on_errorDispatcherSendMessage(static_cast<const errorDispatcherEvent::SendMessage* > (e.get()));
    if( systemEventEnum::startService==ID)
        return on_startService(static_cast<const systemEvent::startService*>(e.get()));
    if( rpcEventEnum::IncomingOnAcceptor==ID)
        return(this->on_IncomingOnAcceptor(static_cast<const rpcEvent::IncomingOnAcceptor*>(e.get())));

    logErr2("ErrorDispatcher: unhandled event %s %s %d",iUtils->genum_name(e->id),__FILE__,__LINE__);
    XPASS;
    return false;
}
