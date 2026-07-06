#pragma once
#include "event_mt.h"
#include "SERVICE_id.h"
#include "IObjectProxy.h"
#include "IInstance.h"
/**
* Base class, which allow any object to be consumer, i.e. receive response of service directly to virtual method
*/
class ObjectHandler
{
public:
    const std::string name;

    /// send event to any service local or remote
    virtual void sendEvent(const std::string & dstHost, const SERVICE_id & dstService, const REF_getter<Event::Base>& e)=0;
    virtual void sendEvent(const msockaddr_in & dstHost, const SERVICE_id & dstService, const REF_getter<Event::Base>& e)=0;
    virtual void sendEvent(const SERVICE_id & dstService, const REF_getter<Event::Base>& e)=0;

    /// reply/pass event according backrouting data
    void passEvent(const REF_getter<Event::Base>& e);

    enum Type
    {
        POLLED,THREADED
    };
    Type type;
    ObjectHandler(const std::string& _name,Type t,IInstance* _if):name(_name),type(t),iInstance(_if)
    {
    }
    virtual ~ObjectHandler()
    {
    }

    /// inherited class receives events into this implemented method
    virtual bool OH_handleObjectEvent(const REF_getter<Event::Base>& e)=0;

protected:
    IInstance *iInstance;


};

/// ObjectHandler polled implementation
class ObjectHandlerPolled: public ObjectHandler
{
    IObjectProxyPolled *objectProxy;
public:
    void sendEvent(const std::string & dstHost, const SERVICE_id & dstService, const REF_getter<Event::Base>& e);
    void sendEvent(const msockaddr_in & dstHost, const SERVICE_id & dstService, const REF_getter<Event::Base>& e);
    void sendEvent(const SERVICE_id & dstService, const REF_getter<Event::Base>& e);


    ObjectHandlerPolled(const std::string& name,IInstance* _if);
    ~ObjectHandlerPolled();

};

/// ObjectHandler threaded implementation
class ObjectHandlerThreaded: public ObjectHandler
{
    IObjectProxyThreaded *objectProxy;
public:
    void sendEvent(const std::string & dstHost, const SERVICE_id & dstService, const REF_getter<Event::Base>& e);
    void sendEvent(const msockaddr_in & dstHost, const SERVICE_id & dstService, const REF_getter<Event::Base>& e);
    void sendEvent(const SERVICE_id & dstService, const REF_getter<Event::Base>& e);


    ObjectHandlerThreaded(const std::string& name,IInstance* _if);
    ~ObjectHandlerThreaded();

};

