#include "objectHandler.h"
#include "IInstance.h"

#include "Events/System/Net/rpcEvent.h"

void ObjectHandlerPolled::sendEvent(const std::string & dstHost, const SERVICE_id & dstService, const REF_getter<Event::Base>& e)
{
    try {
        // Url u;
        // u.parse(dstHost);
        if(dstHost=="local")
        {
            sendEvent(dstService,e);
        }
        else
        {
            msockaddr_in sa;
            sa.init(dstHost);
            sendEvent(sa,dstService,e);
        }
    }
    catch(const CommonError& ex)
    {
        logErr2("CommonError: %s",ex.what());
    }
    catch(const std::exception& ex)
    {
        logErr2("std::exception: %s",ex.what());
    }

}
void ObjectHandlerThreaded::sendEvent(const std::string & dstHost, const SERVICE_id & dstService, const REF_getter<Event::Base>& e)
{
    try {
        // Url u;
        // u.parse(dstHost);
        if(dstHost=="local")
        {
            sendEvent(dstService,e);
        }
        else
        {
            // msockaddr_in sa;
            // sa.init(u);
            sendEvent(dstHost,dstService,e);
        }
    }
    catch(const CommonError& ex)
    {
        logErr2("CommonError: %s",ex.what());
    }
    catch(const std::exception& ex)
    {
        logErr2("std::exception: %s",ex.what());
    }

}

void ObjectHandlerPolled::sendEvent(const msockaddr_in & dstHost, const SERVICE_id & dstService, const REF_getter<Event::Base>& e)
{
    objectProxy->sendObjectRequest(dstHost,dstService,e);
}

void ObjectHandlerThreaded::sendEvent(const msockaddr_in & dstHost, const SERVICE_id & dstService, const REF_getter<Event::Base>& e)
{
    objectProxy->sendObjectRequest(dstHost,dstService,e);
}
void ObjectHandlerPolled::sendEvent(const SERVICE_id & dstService, const REF_getter<Event::Base>& e)
{
    objectProxy->sendObjectRequest(dstService,e);
}
void ObjectHandlerThreaded::sendEvent(const SERVICE_id & dstService, const REF_getter<Event::Base>& e)
{
    objectProxy->sendObjectRequest(dstService,e);
}
ObjectHandlerPolled::ObjectHandlerPolled(const std::string &name, IInstance* _if)
    :ObjectHandler(name,ObjectHandler::POLLED,_if),
     objectProxy(dynamic_cast<IObjectProxyPolled*>
                 (_if->getServiceOrCreate(ServiceEnum::ObjectProxyPolled)))
{
    XTRY;

    if(!objectProxy)
    {
        throw  CommonError("if(!objectProxy)");
    }
    objectProxy->addObjectHandler(this);
    XPASS;
}
ObjectHandlerThreaded::ObjectHandlerThreaded(const std::string& name, IInstance *_if)
    :ObjectHandler(name,ObjectHandler::THREADED,_if),
     objectProxy(dynamic_cast<IObjectProxyThreaded*>
                 (_if->getServiceOrCreate(ServiceEnum::ObjectProxyThreaded)))
{
    XTRY;


    if(!objectProxy)
    {
        logErr2("dynamic_cast<IObjectProxyThreaded*>  (svs); failed");
    }

    if(!objectProxy)
    {
        throw  CommonError("if(!objectProxy)");
    }
    objectProxy->addObjectHandler(this);
    XPASS;
}


ObjectHandlerPolled::~ObjectHandlerPolled()
{

    objectProxy->removeObjectHandler(this);
}
ObjectHandlerThreaded::~ObjectHandlerThreaded()
{
    objectProxy->removeObjectHandler(this);
}

void ObjectHandler::passEvent(const REF_getter<Event::Base>& e)
{
    XTRY;
    logErr2("ObjectHandler::passEvent (%d) %s",__LINE__, iUtils->genum_name(e->id));
    iInstance->passEvent(e);
    XPASS;
}

