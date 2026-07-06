#pragma once

#include "msockaddr_in.h"
#include "SERVICE_id.h"
#include "REF.h"
#include "event_mt.h"
class ObjectHandlerPolled;
class ObjectHandlerThreaded;

/**
 * This interface used to work with events from any C++ object, inherited from ObjectHandler.
 * Ex: you can make any GUI window eventable. It can send event to remote service and receive back event using backroute
*/

class IObjectProxyPolled
{
    /// this interface is used mainly in GUI objects with 1 GUI thread.
public:
    virtual void addObjectHandler(ObjectHandlerPolled* h)=0;
    virtual void removeObjectHandler(ObjectHandlerPolled* h)=0;

    /// send event to remove service over RPC
    virtual void sendObjectRequest(const msockaddr_in & dstHost, const SERVICE_id & dstService, const REF_getter<Event::Base>& e)=0;
    virtual void sendObjectRequest(const SERVICE_id & dstService, const REF_getter<Event::Base>& e)=0;

    /// used in polled event handling, ex. in gui class
    virtual void poll()=0;

    IObjectProxyPolled()
    {
    }

    virtual ~IObjectProxyPolled() {}

};
class IObjectProxyThreaded
{
    /// this interface is used in for threaded app
public:
    virtual void addObjectHandler(ObjectHandlerThreaded* h)=0;
    virtual void removeObjectHandler(ObjectHandlerThreaded* h)=0;

    /// send event to remove service over RPC
    virtual void sendObjectRequest(const msockaddr_in & dstHost, const SERVICE_id & dstService, const REF_getter<Event::Base>& e)=0;
    virtual void sendObjectRequest(const SERVICE_id & dstService, const REF_getter<Event::Base>& e)=0;

    IObjectProxyThreaded()
    {
    }

    virtual ~IObjectProxyThreaded() {}

};
