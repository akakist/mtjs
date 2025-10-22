#include <mutexInspector.h>
#include "objectProxyService.h"
#include <Events/System/Net/rpcEvent.h>
#include <version_mega.h>

#include "objectHandler.h"
#include "events_ObjectProxy.hpp"

bool ObjectProxy::Threaded::handleEvent(const REF_getter<Event::Base>& e)
{
    MUTEX_INSPECTOR;
    XTRY;

    auto &ID=e->id;
    if(systemEventEnum::startService==ID)
        return on_startService((const systemEvent::startService*)e.get());
    if( rpcEventEnum::IncomingOnAcceptor==ID)
        throw CommonError("rpcIncomingOnAcceptor here");
    if( rpcEventEnum::IncomingOnConnector==ID)
    {

        ;
        rpcEvent::IncomingOnConnector* ez=static_cast<rpcEvent::IncomingOnConnector*>(e.get());
        e->route=ez->e->route;
    }

    if(((Event::Base*)e.get())->route.size()==0)
    {
        logErr2("wrong event route %s",iUtils->genum_name(e->id));
    }
    Route r= e.get()->route.pop_back();
    if(r.type==Route::OBJECTHANDLER_THREADED)
    {
        {
            XTRY;

            M_LOCK(__m_lock);
            auto i=__mx_handlers.find(r.objectHandlerRouteThreaded.addr_);
            if(i==__mx_handlers.end())
            {
                ;
                for(auto& it:__mx_handlers)
                {
                    DBG(logErr2("*it !!!"));
                }
                DBG(logErr2("if(!m_handlers.count(l)) l->addr"));
                return false;
            }
            {
                ;
                XTRY;
                ObjectHandlerThreaded* OH=NULL;
                OH=(ObjectHandlerThreaded*)*i;
                try {
                    if( rpcEventEnum::IncomingOnConnector==ID)
                    {
                        rpcEvent::IncomingOnConnector* ez=static_cast<rpcEvent::IncomingOnConnector*>(e.get());
                        ez->e->route=e->route;
                    }
                    M_UNLOCK(__m_lock);
                    if(!OH->OH_handleObjectEvent(e))
                    {
                        logErr2("ObjectProxy::Threaded: unhandled event %s in %s",iUtils->genum_name(e->id), OH->name.c_str());
                    }
                }
                catch(...)
                {
                    logErr2("PASS in %s, %s %d",OH->name.c_str(),__FILE__,__LINE__);
                    throw;
                }
                XPASS;
            }
            XPASS;
        }
    }
    else
    {
        ;
        logErr2("!(r->type==Route::OBJECTHANDLER_THREADED) %d",r.type);
    }
    return true;

    XPASS;
    return false;

}

bool ObjectProxy::Polled::handleEvent(const REF_getter<Event::Base>& e)
{
    XTRY;
    auto &ID=e->id;
    if(systemEventEnum::startService==ID)
        return on_startService((const systemEvent::startService*)e.get());

    if( rpcEventEnum::IncomingOnAcceptor==ID)
        throw CommonError("rpcIncomingOnAcceptor here");
    if( rpcEventEnum::IncomingOnConnector==ID)
    {
        rpcEvent::IncomingOnConnector* ez=static_cast<rpcEvent::IncomingOnConnector*>(e.get());
        e->route=ez->e->route;
    }
    Route r=((Event::Base*)e.get())->route.pop_back();
    if(r.type==Route::OBJECTHANDLER_POLLED)
    {
        {
            ;
            M_LOCK(__m_lock);
            auto i=__mx_handlers.find(r.objectHandlerRoutePolled.addr_);
            if(i==__mx_handlers.end())
            {
                ;
                for(auto& it:__mx_handlers)
                {
                    DBG(logErr2("*it !!!"));
                }
                DBG(logErr2("if(!m_handlers.count(l)) l->addr"));
                return false;
            }
            {
                ;
                ObjectHandlerPolled* OH=NULL;
                OH=(ObjectHandlerPolled*)*i;
                try {
                    ;
                    M_UNLOCK(__m_lock);
                    if(!OH->OH_handleObjectEvent(e))
                    {
                        logErr2("ObjectProxy::Threaded: unhandled event %s in %s", iUtils->genum_name(e->id), OH->name.c_str());
                    }
                }
                catch(...)
                {
                    std::string nm;
                    if(OH)
                        nm=OH->name;
                    logErr2("PASS in %s, %s %d",nm.c_str(),__FILE__,__LINE__);
                }
            }
        }
    }
    else
    {
        logErr2("!(r->type==Route::OBJECTHANDLER_POLLED) ");
    }
    return true;

    XPASS;
    return false;

}
void ObjectProxy::Polled::sendObjectRequest(const msockaddr_in& addr, const SERVICE_id& dst, const REF_getter<Event::Base>& e)
{
    XTRY;
    Route r(Route::LOCALSERVICE);
    r.localServiceRoute.id=ListenerBase::serviceId;
    e->route.push_back(r);
    sendEvent(addr,dst,e);
    XPASS;
}
void ObjectProxy::Polled::sendObjectRequest(const SERVICE_id& dst, const REF_getter<Event::Base>& e)
{
    XTRY;
    Route r(Route::LOCALSERVICE);
    r.localServiceRoute.id=ListenerBase::serviceId;
    e->route.push_back(r);
    sendEvent(dst,e);
    XPASS;
}
void ObjectProxy::Threaded::sendObjectRequest(const msockaddr_in& addr, const SERVICE_id& dst, const REF_getter<Event::Base>& e)
{
    XTRY;
    Route r(Route::LOCALSERVICE);
    r.localServiceRoute.id=ListenerBase::serviceId;
    e->route.push_back(r);
    sendEvent(addr,dst,e);
    XPASS;
}
void ObjectProxy::Threaded::sendObjectRequest(const SERVICE_id& dst, const REF_getter<Event::Base>& e)
{
    XTRY;
    Route r(Route::LOCALSERVICE);
    r.localServiceRoute.id=ListenerBase::serviceId;
    e->route.push_back(r);
    sendEvent(dst,e);
    XPASS;
}

void registerObjectProxyModule(const char* pn)
{
    XTRY;
    if(pn)
    {
        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_SERVICE,ServiceEnum::ObjectProxyPolled,"ObjectProxyPolled",getEvents_ObjectProxy());
        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_SERVICE,ServiceEnum::ObjectProxyThreaded,"ObjectProxyThreaded",getEvents_ObjectProxy());
    }
    else
    {
        iUtils->registerService(ServiceEnum::ObjectProxyPolled,ObjectProxy::Polled::construct,"ObjectProxyPolled");
        iUtils->registerService(ServiceEnum::ObjectProxyThreaded,ObjectProxy::Threaded::construct,"ObjectProxyThreaded");
        regEvents_ObjectProxy();
    }
    XPASS;
}
void ObjectProxy::Polled::addObjectHandler(ObjectHandlerPolled* h)
{
    ;
    XTRY;
    M_LOCK(__m_lock);
    __mx_handlers.insert(h);
    XPASS;
}
void ObjectProxy::Polled::removeObjectHandler(ObjectHandlerPolled* h)
{
    XTRY;
    M_LOCK(__m_lock);
    __mx_handlers.erase(h);
    XPASS;
}

void ObjectProxy::Threaded::addObjectHandler(ObjectHandlerThreaded* h)
{
    XTRY;
    M_LOCK(__m_lock);
    __mx_handlers.insert(h);
    XPASS;
}
void ObjectProxy::Threaded::removeObjectHandler(ObjectHandlerThreaded* h)
{
    XTRY;
    M_LOCK(__m_lock);
    __mx_handlers.erase(h);
    XPASS;
}

