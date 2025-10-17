#pragma once

#include "IUtils.h"
#include <IInstance.h>
#include <listenerBase.h>

class CInstance: public IInstance
{
    Mutex initService_MX;
    std::map<SERVICE_id,Mutex*> mx_initInProcess;
    UnknownBase* initService(const SERVICE_id& sid);
    std::string name;
public:
    void setConfig(IConfigObj *value)
    {
        config_z=value;
    }
    CInstance( IUtils *_m_utils, const std::string& _name

             );
    ~CInstance()
    {
        m_terminating=true;
        deinitServices();
        delete config_z;
        for(auto z: mx_initInProcess)
        {
            delete z.second;
        }
    }
    std::string getName() {
        return name;
    }




    void forwardEvent(const SERVICE_id& svs,  const REF_getter<Event::Base>&e);

    /// send event - direction from consumer to server
    void sendEvent(const SERVICE_id& svs,  const REF_getter<Event::Base>&e);
    void sendEvent(ListenerBase* svs,  const REF_getter<Event::Base>&e);
    void sendEvent(const msockaddr_in& addr, const SERVICE_id& svs,  const REF_getter<Event::Base>&e);
    void sendEvent(const std::string & addr, const SERVICE_id& svs,  const REF_getter<Event::Base>&e);



    //// pass event - directon from server to consumer
    void passEvent(const REF_getter<Event::Base>&e);

    void initServices();
    void deinitServices();

    UnknownBase* getServiceOrCreate(const SERVICE_id& id);
    UnknownBase* getServiceNoCreate(const SERVICE_id& id);

    IConfigObj* getConfig()
    {
        return config_z;
    }
private:

    IConfigObj* config_z;
    IUtils *m_utils;

    struct __services
    {
        RWLock m_lock;
        std::map<SERVICE_id,UnknownBase* > container;
    };
    __services services;

    Mutex mx_globalCookie;
    GlobalCookie_id m_globalCookie;
    GlobalCookie_id globalCookie();


public:

    RWLock mx_myExternalAddresses;
    std::set<msockaddr_in> myExternalAddresses;

    std::set<msockaddr_in> myExternalAddressesGet()
    {
        R_LOCK(mx_myExternalAddresses);
        return myExternalAddresses;
    }
    void myExternalAddressesInsert(const msockaddr_in&s)
    {
        W_LOCK(mx_myExternalAddresses);
        myExternalAddresses.insert(s);
    }
    size_t myExternalAddressesCount(const msockaddr_in&s)
    {
        R_LOCK(mx_myExternalAddresses);
        return myExternalAddresses.count(s);

    }
    size_t myExternalAddressesSize()
    {
        R_LOCK(mx_myExternalAddresses);
        return myExternalAddresses.size();

    }

    bool m_terminating;


};

