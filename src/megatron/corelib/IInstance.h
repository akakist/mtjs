#pragma once
#include <string>
#include "IConfigObj.h"
#include "event_mt.h"
#include "SERVICE_id.h"
#include "unknown.h"
#include "GlobalCookie_id.h"
#ifndef __ANDROID__
#endif

///       Service sandbox management

class ListenerBase;
class IInstance
{
public:


    /// put event to service deque, init service if needed
    virtual void forwardEvent(const SERVICE_id& svs,  const REF_getter<Event::Base>&e)=0;

    /// dispatch event depending on destination service type
    virtual void sendEvent(const SERVICE_id& svs,  const REF_getter<Event::Base>&e)=0;

    /// dipatch event into remote service over RPC
    virtual void sendEvent(const msockaddr_in& addr, const SERVICE_id& svs,  const REF_getter<Event::Base>&e)=0;
    virtual void sendEvent(const std::string& addr, const SERVICE_id& svs,  const REF_getter<Event::Base>&e)=0;
    virtual void sendEvent(ListenerBase *l,  const REF_getter<Event::Base>&e)=0;


    /// send "back" event (ex: server response)
    virtual void passEvent(const REF_getter<Event::Base>&e)=0;


    ///  init services using config settings. Mainly server is created if events sent for it.
    /// But like RPC service needed to be inited to accept incoming events
    virtual void initServices()=0;

    /// mark services to stop, wait it's termination
    virtual void deinitServices()=0;

    /// init one service
    virtual UnknownBase* initService(const SERVICE_id& sid)=0;



    /// get pointer to service, initialize it if not running
    virtual UnknownBase*  getServiceOrCreate(const SERVICE_id& id)=0;

    /// get pointer to service without init
    virtual UnknownBase* getServiceNoCreate(const SERVICE_id& id)=0;

    /// get config for current sanbox
    virtual IConfigObj* getConfig()=0;

    virtual ~IInstance() {}


    /// global cookie - random ID identifying node in DFS cloud
    virtual GlobalCookie_id globalCookie()=0;

    /// my visible IP adresses, obtained with cloud uplink
    virtual std::set<msockaddr_in> myExternalAddressesGet()=0;
    virtual void myExternalAddressesInsert(const msockaddr_in&s)=0;
    virtual size_t myExternalAddressesCount(const msockaddr_in&s)=0;
    virtual size_t myExternalAddressesSize()=0;

    /// set config for this sandbox
    virtual void setConfig(IConfigObj *value)=0;

    virtual std::string getName()=0;

};


