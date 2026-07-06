#include "ISSL.h"
#include "CInstance.h"




#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/Net/rpcEvent.h"
#include "utils_local.h"
#include "colorOutput.h"


#ifndef _MSC_VER
#endif

CInstance::CInstance(IUtils *_m_utils, const std::string& _name
                    )

    :
    name(_name),
    config_z(nullptr),
    m_utils(_m_utils),
    m_terminating(false)
{

}





void CInstance::passEvent(const REF_getter<Event::Base>&e)
{
    MUTEX_INSPECTOR;

    if(m_terminating)return;
    XTRY;
    Route r=e->route.pop_back();
    switch(r.type)
    {
    case Route::LOCALSERVICE:
    {
        MUTEX_INSPECTOR;
        // LocalServiceRoute* rt=(LocalServiceRoute* )r.get();
        forwardEvent(r.localServiceRoute.id,e);
    }
    break;
    case Route::LISTENER:
    {
        MUTEX_INSPECTOR;
        // ListenerRoute* rt=(ListenerRoute* )r.get();
        r.listenerRoute.id->listenToEvent(e);
    }
    break;
    case Route::OBJECTHANDLER_POLLED:
    {
        MUTEX_INSPECTOR;
        e->route.push_back(r);
        forwardEvent(ServiceEnum::ObjectProxyPolled,e);
        break;
    }
    case Route::OBJECTHANDLER_THREADED:
    {
        MUTEX_INSPECTOR;
        e->route.push_back(r);
        forwardEvent(ServiceEnum::ObjectProxyThreaded,e);
        break;
    }
    case Route::REMOTEADDR:
    {
        MUTEX_INSPECTOR;

        XTRY;
        if(!e.valid())
            throw CommonError("if(!e.valid()) %s",_DMI().c_str());
        // RemoteAddrRoute* rt=(RemoteAddrRoute* )r.get();
        forwardEvent(ServiceEnum::RPC,new rpcEvent::PassPacket(r.remoteAddrRoute.addr,e));
        XPASS;

    }
    break;
    default:
        throw CommonError("passEvent: invalid route type %d",r.type);
    }
    XPASS;

}
void CInstance::sendEvent(const SERVICE_id& svs,  const REF_getter<Event::Base>&e)
{
    if(m_terminating)return;
    XTRY;
    Route r(Route::LOCALSERVICE);
    r.localServiceRoute.id=svs;
    e->route.push_back(r);
    forwardEvent(svs,e);
    XPASS;
}
void CInstance::sendEvent(const std::string& dstHost, const SERVICE_id& dstService,  const REF_getter<Event::Base>&e)
{
    if(m_terminating)return;
    XTRY;
    if(dstHost=="local")
    {
        sendEvent(dstService,e);
    }
    else
    {
        try {
            // Url u;
            // u.parse(dstHost);

            msockaddr_in sa;
            sa.init(dstHost);
            sendEvent(sa,dstService,e);
        }
        catch(const std::exception& ex)
        {
            logErr2("exception: %s",ex.what());
        }
        // catch(const CommonError& ex)
        // {
        //     logErr2("exception: %s",ex.what());
        // }
    }

    XPASS;
}
void CInstance::sendEvent(const msockaddr_in& addr, const SERVICE_id& svs,  const REF_getter<Event::Base>&e)
{
//    if(m_terminating)return;
    XTRY;
    forwardEvent(ServiceEnum::RPC,new rpcEvent::SendPacket(addr,svs,e));
    XPASS;
}

void CInstance::sendEvent(ListenerBase *l,  const REF_getter<Event::Base>&e)
{
//    if(m_terminating)return;
    Route r(Route::LISTENER);
    r.listenerRoute.id=l;
    e->route.push_back(r);
    l->listenToEvent(e);
}
UnknownBase* CInstance::getServiceNoCreate(const SERVICE_id& svs)
{
//    if(m_terminating)return nullptr;
    R_LOCK(services.m_lock);
    auto i=services.container.find(svs);
    UnknownBase* u=NULL;
    if(i==services.container.end())
    {
        u=NULL;
    }
    else
    {
        u=i->second;
    }
    return u;
}

UnknownBase* CInstance::getServiceOrCreate(const SERVICE_id& svs)
{
    if(m_terminating)
    {
        return nullptr;
    }

    XTRY;
    UnknownBase* u=nullptr;
    {
        XTRY;
        R_LOCK(services.m_lock);
        auto i=services.container.find(svs);
        if(i==services.container.end())
        {
            u=NULL;
        }
        else
        {
            u=i->second;
        }
        XPASS;
    }
    XTRY;
    if(!u)
    {
        u=initService(svs);
    }
    XPASS;
    XTRY;
    if(u)
    {
        return u;
    }
    XPASS;
    XPASS;
    throw CommonError("cannot find service %s",iUtils->genum_name(svs));

}

void CInstance::forwardEvent(const SERVICE_id& svs,  const REF_getter<Event::Base>&e)
{

    if(m_terminating)return;
    XTRY;
    try
    {
        UnknownBase*u=nullptr;
        {
            XTRY;
            R_LOCK(services.m_lock);
            auto i=services.container.find(svs);
            if(i==services.container.end())
            {
                u=NULL;
            }
            else
            {
                u=i->second;
            }
            XPASS;
        }
        if(!u)
        {
            MUTEX_INSPECTOR;
            XTRY;
            u=initService(svs);

            if(!u)
                throw CommonError("Service '%s' not registered",iUtils->serviceName(svs).c_str());
            XPASS;
        }
        if(u!= nullptr)
        {
            XTRY;
            auto l=dynamic_cast<ListenerBase*>(u);
            if(!l)
                throw CommonError("if(!l)");

            l->listenToEvent(e);
            XPASS;
        }
        else throw CommonError("!U -------------");
    }
    catch(const CommonError &err)
    {
        logErr2("CommonError catched for event %s %s",iUtils->genum_name(e->id),err.what());
    }
    catch(const std::exception &err)
    {
        logErr2("std::exception catched for event %s %s",iUtils->genum_name(e->id),err.what());
    }

    XPASS;
}
UnknownBase* CInstance::initService(const SERVICE_id& sid)
{
    MUTEX_INSPECTOR;
    {
        if(m_terminating)return NULL;
        Utils_local * locals = m_utils->getLocals();
        Mutex *initMX=NULL;
        {
            M_LOCK(initService_MX);
            if(!mx_initInProcess.count(sid)) {
                mx_initInProcess[sid] = new Mutex;
            }

            initMX=mx_initInProcess[sid];
        }

        M_LOCK(initMX);


        XTRY;
        UnknownBase* u=NULL;

        unknown_static_constructor constr=0;

        {
            auto i=services.container.find(sid);
            if(i!=services.container.end())
                return i->second;
        }
        {
            MUTEX_INSPECTOR;
            R_LOCK(locals->service_constructors.lk);
            auto i=locals->service_constructors.container.find(sid);
            if(i==locals->service_constructors.container.end())
            {
            }
            else
            {
                constr=i->second;
            }
        }
        if(constr==0)
        {
            MUTEX_INSPECTOR;
            std::string pn;
            {
                MUTEX_INSPECTOR;
                {
                    M_LOCK(locals->pluginInfo);
                    auto i=locals->pluginInfo.services.find(sid);
                    if(i!=locals->pluginInfo.services.end())
                    {
                        pn=i->second;
                    }
                }
                if(!pn.size())
                    throw CommonError("cannot find plugin for service  (sid %s)",iUtils->genum_name(sid));

            }
            m_utils->registerPluginDLL(pn);
            {
                MUTEX_INSPECTOR;
                R_LOCK(locals->service_constructors.lk);
                auto i=locals->service_constructors.container.find(sid);
                if(i==locals->service_constructors.container.end())
                {
                    XTRY;
                    // M_UNLOCK(locals->service_constructors);
                    throw CommonError("cannot load service ServiceId %s",iUtils->genum_name(sid));
                    XPASS;
                }
                else
                {
                    constr=i->second;
                }
            }
        }
        if(m_terminating)return NULL;
        {
            {
                MUTEX_INSPECTOR;
                R_LOCK(services.m_lock);
                auto iii=services.container.find(sid);
                if(iii!=services.container.end())
                {
                    return iii->second;
                }
            }
//        else
            {
                MUTEX_INSPECTOR;
                {
                    MUTEX_INSPECTOR;
//                M_UNLOCK(services);
                    std::string name;
                    {
                        MUTEX_INSPECTOR;
                        name=iUtils->serviceName(sid);
                    }
                    DBG(logErr2("running service %s",name.c_str()));
                    if(!config_z)
                        throw CommonError("if(!config_z)");
                    CFG_PUSH(name,config_z);
                    {
                        MUTEX_INSPECTOR;
                        XTRY;
                        if(constr==0) {
                            MUTEX_INSPECTOR;
                            XTRY;
                            throw CommonError("if(constr==0)");
                            XPASS;
                        }
                        XTRY;
                        u=constr(sid,name.c_str(),this);
                        XPASS;
                        XPASS;
                    }
                }
                {
                    W_LOCK(services.m_lock);
                    services.container.insert(std::make_pair(sid,u));
                }
                if(m_terminating)return NULL;
            }
        }
        try {
            if(!u)
                throw CommonError("if(!u)");
            auto l=dynamic_cast<ListenerBase*>(u);
            if(!l)
                throw CommonError("if(!l)");
            l->listenToEvent(new systemEvent::startService);
        }
        catch(const CommonError &e)
        {
            logErr2("startService commonError %s",e.what());
            return NULL;
        }
        catch(const std::exception &e)
        {
            logErr2("startService std::exception %s",e.what());
            return NULL;
        }
        return u;
        XPASS;
    }
}
void CInstance::initServices()
{
    MUTEX_INSPECTOR;
    XTRY;
    if(m_terminating)return;
    std::vector<std::string> svs;
#if !defined __CLI__
    svs.push_back("RPC");
#endif

    std::set<std::string> run=config_z->get_stringset("Start",iUtils->join(",",svs),"list of initially started services");
    for(auto& i:run)
    {
        if(m_terminating)return;
        MUTEX_INSPECTOR;
        SERVICE_id sid;
//        logErr2("start service %s",i.c_str());
        try {
            sid=iUtils->serviceIdByName(i);
        }
        catch(const std::exception& e)
        {
            logErr2("serviceIdByName: Service name not found %s",i.c_str());
            return;
        }
        getServiceOrCreate(sid);
    }

    XPASS;
}
void CInstance::deinitServices()
{
    MUTEX_INSPECTOR;

    std::map<SERVICE_id,UnknownBase* >svs;
    {
        MUTEX_INSPECTOR;
        {
            W_LOCK(services.m_lock);
            svs=services.container;
            services.container.clear();
        }
    }
    for(auto i=svs.begin(); i!=svs.end(); i++)
    {

        std::string name;
        {
            auto u=i->second;
            MUTEX_INSPECTORS(u->classname);
            name=u->classname;
            DBG(printf(BLUE("deleting service %s %s"),u->classname.c_str(),_DMI().c_str()));
            u->deinit();
            try {
                delete u;
            }
            catch(...)
            {

            }
        }
        DBG(printf(BLUE("deleted service %s"),name.c_str()));
    }
}






GlobalCookie_id CInstance::globalCookie()
{
    M_LOCK(mx_globalCookie);
    if(CONTAINER(m_globalCookie).size()==0)
    {
        I_ssl *ssl=(I_ssl*)iUtils->queryIface(Ifaces::SSL);
        CONTAINER(m_globalCookie).resize(8);
        for(int i=0; i<8; i++)
            CONTAINER(m_globalCookie)[i]=rand();

    }
    return m_globalCookie;
}
