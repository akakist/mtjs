#include "mutexInspector.h"
#include <unistd.h>
#define STATICLIB 1

#include "rpcService.h"

#include <Events/System/Net/rpcEvent.h>
#include "Events/System/Net/oscarEvent.h"
#include "Events/System/timerEvent.h"

#include "logging.h"
#include "tools_mt.h"
#include "events_rpc.hpp"
RPC::Service::Service(const SERVICE_id &svs, const std::string&  nm, IInstance* ifa):
    UnknownBase(nm),
    ListenerSimple(nm,svs),
    Broadcaster(ifa),
    // myOscar(ifa->getConfig()->get_string("oscarType","Oscar","oscar type - Oscar || OscarSecure")),
    iInstance(ifa),
    m_isTerminating(false)
{

    // secure.use_ssl=ifa->getConfig()->get_bool("use_ssl",false,"secure connection");
    try
    {

        XTRY;
        {
            std::string _main,_reserve;
#if defined(__ANDROID__) || defined(__IOS__) || defined(QT_CORE_LIB)
            _main="NONE";
#else
            _main="INADDR_ANY:0,INADDR6_ANY:0";
#endif
            _reserve="NONE";
        }

        XPASS;
    }
    catch (const std::exception &e)
    {
        logErr2("exception: %s %d",e.what(),__LINE__);
        throw;
    }
}
RPC::Service::~Service()
{
    m_isTerminating=true;
    sessions.clear();

}

bool RPC::Service::on_Connected(const oscarEvent::Connected* E)
{
    MUTEX_INSPECTOR;

    REF_getter<Session> S(NULL);
    msockaddr_in addr;
    {
        if(!E->esi->request_for_connect_.has_value())
            throw CommonError("if(!E->esi->request_for_connect.has_value())");
        R_LOCK(sessions.sa2sess_mx.lk);
        auto it=sessions.sa2sess_mx.container_.find(*E->esi->request_for_connect_);
        if(it==sessions.sa2sess_mx.container_.end())
        {
            throw CommonError("2cannot find RPC session on connected %s",E->esi->request_for_connect_->dump().c_str());
            return true;
        }
        S=it->second;
    }

    {
        std::vector<REF_getter<Event::Base>>d;
        {
            R_LOCK(sessions.subscribers_mx.lk);
            d.reserve(sessions.subscribers_mx.container_.size());
            for(auto &i:sessions.subscribers_mx.container_)
            {
                d.push_back(new rpcEvent::Connected(E->esi,addr,i));
            }
        }
        for(auto& z: d)
            passEvent(z);
    }

    if(!S.valid()) throw CommonError("!!!!! FATAL invalid S");

    S->esi=E->esi;
    flushOutCache(S);
    return true;
}
bool RPC::Service::on_PacketOnAcceptor(const oscarEvent::PacketOnAcceptor*E)
{
    MUTEX_INSPECTOR;
    try {
        inBuffer buf(E->buf);
        int direction;
        buf>>direction;

        if(direction=='p')
        {
            MUTEX_INSPECTOR;
            REF_getter<Event::Base> e=iUtils->unpackEvent(buf);
            if(!e.valid())
            {
                return true;
            }
            Route r=e->route.pop_back();
            if(r.type==Route::LOCALSERVICE)
            {
                sendEvent(r.localServiceRoute.id,new rpcEvent::IncomingOnAcceptor(E->esi,e));
            }
            else
            {
                throw CommonError("!(r->type==Route::LOCALSERVICE) %s",_DMI().c_str());
            }
        }
        else if(direction=='s')
        {
            MUTEX_INSPECTOR;
            SERVICE_id dst;
            buf>>dst;
            REF_getter<Event::Base> e=iUtils->unpackEvent(buf);
            if(!e.valid())
            {
                return true;
            }
            Route r1(Route::REMOTEADDR);
            r1.remoteAddrRoute.addr=E->esi->id_;
            e->route.push_back(r1);
            Route r2(Route::LOCALSERVICE);
            r2.localServiceRoute.id=dst;
            e->route.push_back(r2);
            sendEvent(dst,new rpcEvent::IncomingOnAcceptor(E->esi,e));
        }
    }
    catch(const std::exception &e)
    {
        DBG(logErr2("PKT RPC catched %s",e.what()));
        E->esi->close("broken packet in RPC");
    }

    return true;
}


bool RPC::Service::on_PacketOnConnector(const oscarEvent::PacketOnConnector* E)
{

    MUTEX_INSPECTOR;

    try {
        MUTEX_INSPECTOR;
        inBuffer buf(E->buf);
        int direction;
        buf>>direction;


        if(direction=='p')
        {
            MUTEX_INSPECTOR;
            REF_getter<Event::Base> e=iUtils->unpackEvent(buf);

            if(!e.valid())
            {
                MUTEX_INSPECTOR;
                logErr2("if(!e.valid())");
                return true;
            }


            Route r=e->route.pop_back();
            if(r.type==Route::LOCALSERVICE)
            {
                if(!E->esi->request_for_connect_.has_value())
                {
                    throw CommonError("if(!E->esi->request_for_connect_.has_value())");
                }
                sendEvent(r.localServiceRoute.id,new rpcEvent::IncomingOnConnector(E->esi,*E->esi->request_for_connect_,e));
            }
            else
            {
                throw CommonError("!(r->type==Route::LOCALSERVICE %d) %s %s %s ",
                                  r.type,iUtils->genum_name(e->id), iUtils->genum_name(e->id),_DMI().c_str());
            }
        }
        else if(direction=='s')
        {
            MUTEX_INSPECTOR;
            SERVICE_id dst;
            buf>>dst;
            REF_getter<Event::Base> e=iUtils->unpackEvent(buf);
            if(!e.valid())
            {
                logErr2("if(!e.valid())");
                return true;
            }

            Route r1(Route::REMOTEADDR);
            r1.remoteAddrRoute.addr=E->esi->id_;
            e->route.push_back(r1);
            Route r2(Route::LOCALSERVICE);
            r2.localServiceRoute.id=dst;
            e->route.push_back(r2);
            if(!E->esi->request_for_connect_.has_value())
                throw CommonError("if(!E->esi->request_for_connect.valid())");
            {
                sendEvent(dst,new rpcEvent::IncomingOnConnector(E->esi,*E->esi->request_for_connect_,e));
            }
        }
        else throw CommonError("invalid direction");
    }
    catch(const std::exception &e)
    {
        DBG(logErr2("PKT RPC catched %s",e.what()));
        E->esi->close("broken packet int RPC");
    }
    return true;

}
void RPC::Service::flushOutCache(const REF_getter<Session> & S)
{
    std::deque<REF_getter<refbuffer> > d;
    {
        W_LOCK(S->outCache_.lk);
        d=std::move(S->outCache_.container_lk);
    }
    for(auto &p:d)
    {
        if(!S->esi.valid())
            throw CommonError("if(!S->esi.valid())");
        sendEvent(myOscarListener,new oscarEvent::SendPacket(S->esi,p,dynamic_cast<ListenerBase*>(this)));
    }

}

void RPC::Service::addSendPacket(const REF_getter<Session>&S, const REF_getter<refbuffer>&P)
{
    MUTEX_INSPECTOR;
    S_LOG("addSendPacket");
    bool need_cache=false;

    if(!S.valid())
        need_cache=true;
    if(!need_cache && !S->esi.valid())
        need_cache=true;
    if(need_cache)
    {
        W_LOCK(S->outCache_.lk);
        S->outCache_.container_lk.push_back(P);
        return;
    }
    else
    {

        flushOutCache(S);
        if(!S->esi.valid())
            throw CommonError("if(!S->esi.valid())");
        sendEvent(myOscarListener,new oscarEvent::SendPacket(S->esi,P,dynamic_cast<ListenerBase*>(this)));
    }
}
bool RPC::Service::on_PassPacket(const rpcEvent::PassPacket* E)
{

    MUTEX_INSPECTOR;

    XTRY;
    if(!E)
        return false;
    outBuffer o;
    o<<'p';
    iUtils->packEvent(o,E->e);

    REF_getter<Session> S(NULL);
    {
        R_LOCK(sessions.sock2sess_mx.lk);
        auto it=sessions.sock2sess_mx.container_.find(E->socketIdTo);
        if(it!=sessions.sock2sess_mx.container_.end())
        {
            S=it->second;
        }
    }

    if(S.valid())
    {
        addSendPacket(S,o.asString());
    }
    else {
        W_LOCK (sessions.passCache_mx.lk);
        sessions.passCache_mx.passCache[E->socketIdTo].push_back(o.asString());
    }

    XPASS;
    return true;
}

bool RPC::Service::on_NotifyBindAddress(const oscarEvent::NotifyBindAddress*e)
{
    MUTEX_INSPECTOR;

    {
        W_LOCK(sharedAddr.lk);
        sharedAddr.m_bindAddr_mainSH.insert(e->addr);
    }
    flushAll();
    return true;
}
bool RPC::Service::DoListen(const rpcEvent::DoListen* E)
{
    SOCKET_id newid=iUtils->getNewSocketId();
    sendEvent(myOscarListener,new oscarEvent::AddToListenTCP(newid,E->addr,"RPC",E->secure, dynamic_cast<ListenerBase*>(this)));

    return true;
}

bool RPC::Service::on_startService(const systemEvent::startService* )
{
    MUTEX_INSPECTOR;
    XTRY;

    myOscarListener=dynamic_cast<ListenerBase*>(iInstance->getServiceOrCreate(ServiceEnum::Oscar));


#ifdef WEBDUMP
    sendEvent(ServiceEnum::WebHandler, new webHandlerEvent::RegisterDirectory("rpc","RPC"));
    sendEvent(ServiceEnum::WebHandler, new webHandlerEvent::RegisterHandler("rpc/details","Dump state",ListenerBase::serviceId));
#endif


    XPASS;
    return true;
}
bool RPC::Service::on_Accepted(const oscarEvent::Accepted* E)
{
    MUTEX_INSPECTOR;
    S_LOG("on_Accepted");
    {
        {
            REF_getter<Session> S=new Session(E->esi->id_,E->esi);
            std::deque<REF_getter<refbuffer>> d;
            {
                W_LOCK(sessions.sock2sess_mx.lk);
                sessions.sock2sess_mx.container_.emplace(E->esi->id_,S);
            }
            {
                W_LOCK(sessions.passCache_mx.lk);
                d=std::move(sessions.passCache_mx.passCache[E->esi->id_]);
                sessions.passCache_mx.passCache.erase(E->esi->id_);
            }

            for(auto &dd: d)
            {
                addSendPacket(S,dd);
            }

        }
    }
    {
        std::vector<REF_getter<Event::Base>> d;
        {
            R_LOCK(sessions.subscribers_mx.lk);
            d.reserve(sessions.subscribers_mx.container_.size());
            for(auto &i:sessions.subscribers_mx.container_)
            {
                d.push_back(new rpcEvent::Accepted(E->esi,i));

            }

        }
        for(auto& z:d)
            passEvent(z);
    }


    return true;
}

bool RPC::Service::on_SendPacket(const rpcEvent::SendPacket* E)
{

    MUTEX_INSPECTOR;
    S_LOG("on_SendPacket");
    XTRY;
    bool found=false;

    REF_getter<Session> S(NULL);
    {
        {
            R_LOCK(sessions.sa2sess_mx.lk);
            auto it=sessions.sa2sess_mx.container_.find(E->addressTo);
            if(it!=sessions.sa2sess_mx.container_.end())
                S=it->second;
        }

        if(!S.valid())
        {
            SOCKET_id sockId=iUtils->getNewSocketId();
            msockaddr_in addressTo=E->addressTo;
            S=new Session(sockId,NULL);
            {
                W_LOCK(sessions.sa2sess_mx.lk);
                sessions.sa2sess_mx.container_.insert({E->addressTo,S});
            }
            sendEvent(myOscarListener,new oscarEvent::Connect(sockId,E->addressTo,"RPC", secure, dynamic_cast<ListenerBase*>(this)));

        }
    }
    {
        XTRY;
        outBuffer o;
        o<<'s';
        o<<E->destination;
        iUtils->packEvent(o,E->ev);
        addSendPacket(S,o.asString());
        XPASS;
    }
    XPASS;
    return true;
}
bool RPC::Service::on_NotifyOutBufferEmpty(const oscarEvent::NotifyOutBufferEmpty* e)
{
    MUTEX_INSPECTOR;
    return true;
}



UnknownBase* RPC::Service::construct(const SERVICE_id& id, const std::string&  nm,IInstance* ifa)
{
    XTRY;
    return new Service(id,nm,ifa);
    XPASS;
}

bool RPC::Service::on_UnsubscribeNotifications(const rpcEvent::UnsubscribeNotifications* E)
{
    W_LOCK(sessions.subscribers_mx.lk );
    sessions.subscribers_mx.container_.erase(E->route);
    return true;
}

bool RPC::Service::on_SubscribeNotifications(const rpcEvent::SubscribeNotifications* E)
{
    W_LOCK(sessions.subscribers_mx.lk );
    sessions.subscribers_mx.container_.insert(poppedFrontRoute(E->route));
    return true;
}


void registerRPCService(const char* pn)
{
    if(pn)
    {
        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_SERVICE,ServiceEnum::RPC,"RPC",getEvents_rpc());
    }
    else
    {
        iUtils->registerService(ServiceEnum::RPC,RPC::Service::construct,"RPC");
        regEvents_rpc();
    }
}



bool RPC::Service::on_TickAlarm(const timerEvent::TickAlarm*e)
{
    MUTEX_INSPECTOR;
    return true;
}

unsigned short RPC::Service::getExternalListenPortMain()
{
    MUTEX_INSPECTOR;
    bool cond=false;
    {
        R_LOCK(sharedAddr.lk);
        cond=sharedAddr.m_bindAddr_mainSH.empty();
    }
    while(cond)
    {
        if(m_isTerminating)
        {
            return 0;
        }
        usleep(10000);
        {
            RLocker(sharedAddr.lk);
            cond=sharedAddr.m_bindAddr_mainSH.empty();

        }

    }
    {
        R_LOCK(sharedAddr.lk);
        if(sharedAddr.m_bindAddr_mainSH.size()==0)
        {
            return 0;
        }
        return sharedAddr.m_bindAddr_mainSH.begin()->port();
    }
}
std::set<msockaddr_in> RPC::Service::getInternalListenAddrs()
{
    MUTEX_INSPECTOR;
    bool cond=false;
    {
        R_LOCK(sharedAddr.lk);
        cond= sharedAddr.m_bindAddr_mainSH.empty();
    }
    while(cond)
    {
        usleep(10000);
        {
            R_LOCK(sharedAddr.lk);
            cond= sharedAddr.m_bindAddr_mainSH.empty();
        }
    }
    R_LOCK(sharedAddr.lk);
    return sharedAddr.m_internalAddr;

}
void RPC::Service::flushAll()
{
    std::set<REF_getter<Session> > s;
    {
        R_LOCK(sessions.sock2sess_mx.lk);
        for(auto& z: sessions.sock2sess_mx.container_)
            s.insert(z.second);
    }
    {
        R_LOCK(sessions.sa2sess_mx.lk);
        for(auto& z: sessions.sa2sess_mx.container_)
            s.insert(z.second);
    }
    for(auto &i:s)
    {
        if(!i->esi.valid())
            continue;

        flushOutCache(i);
    }

}
bool RPC::Service::on_ConnectFailed(const oscarEvent::ConnectFailed*e)
{
    MUTEX_INSPECTOR;
    if(!e->esi->request_for_connect_.has_value())
        throw CommonError("if(!e->esi->request_for_connect.valid())");

    {
        W_LOCK(sessions.sa2sess_mx.lk);
        sessions.sa2sess_mx.container_.erase(*e->esi->request_for_connect_);
    }

    {
        std::vector<REF_getter<Event::Base>> d;
        {
            R_LOCK(sessions.subscribers_mx.lk);
            for(auto &i:sessions.subscribers_mx.container_)
            {
                d.push_back(new rpcEvent::ConnectFailed(e->addr,i));
            }
        }
        for(auto& z:d)
        {
            passEvent(z);
        }
    }
    return true;
}
bool RPC::Service::on_Disconnected(const oscarEvent::Disconnected* e)
{
    MUTEX_INSPECTOR;
    {
        std::vector<REF_getter<Event::Base>> d;
        {
            R_LOCK(sessions.subscribers_mx.lk);
            d.reserve(sessions.subscribers_mx.container_.size());
            for(auto &i:sessions.subscribers_mx.container_)
            {

                DBG(logErr2("-------------DISCONNECTED SUCCESSED"));
                if(!e->esi->request_for_connect_.has_value())
                    throw CommonError("if(!e->esi->request_for_connect.valid())");
                d.push_back(new rpcEvent::Disconnected(e->esi,*e->esi->request_for_connect_,e->reason,i));
            }
        }
        for(auto &z:d)
        {
            passEvent(z);
        }
    }
    {
        W_LOCK(sessions.sa2sess_mx.lk);
        sessions.sa2sess_mx.container_.erase(*e->esi->request_for_connect_);
    }

    return true;
}
bool RPC::Service::on_Disaccepted(const oscarEvent::Disaccepted*e)
{
    MUTEX_INSPECTOR;

    {
        std::vector<REF_getter<Event::Base>> d;
        {
            R_LOCK(sessions.subscribers_mx.lk);
            d.reserve(sessions.subscribers_mx.container_.size());
            for(auto &i:sessions.subscribers_mx.container_)
            {
                d.push_back(new rpcEvent::Disaccepted(e->esi,e->reason,i));
            }
        }
        for(auto& z: d)
        {
            passEvent(z);

        }
    }
    {
        W_LOCK(sessions.sock2sess_mx.lk);
        sessions.sock2sess_mx.container_.erase(e->esi->id_);
    }


    return true;

}

bool RPC::Service::handleEvent(const REF_getter<Event::Base>& e)
{
    MUTEX_INSPECTOR;
    XTRY;
    //auto _this=this;
    auto& ID=e->id;
    switch(ID)
    {
    case timerEventEnum::TickAlarm:
        return on_TickAlarm((const timerEvent::TickAlarm*)e.get());
    case timerEventEnum::TickTimer:
        return true;
    case oscarEventEnum::PacketOnAcceptor:
        return(this->on_PacketOnAcceptor((const oscarEvent::PacketOnAcceptor*)e.get()));
    case oscarEventEnum::PacketOnConnector:
        return(this->on_PacketOnConnector((const oscarEvent::PacketOnConnector*)e.get()));
    case oscarEventEnum::Connected:
        return(this->on_Connected((const oscarEvent::Connected*)e.get()));
    case oscarEventEnum::Accepted:
        return(this->on_Accepted((const oscarEvent::Accepted*)e.get()));
    case oscarEventEnum::NotifyBindAddress:
        return(this->on_NotifyBindAddress((const oscarEvent::NotifyBindAddress*)e.get()));
    case oscarEventEnum::NotifyOutBufferEmpty:
        return(this->on_NotifyOutBufferEmpty((const oscarEvent::NotifyOutBufferEmpty*)e.get()));
    case oscarEventEnum::ConnectFailed:
        return(this->on_ConnectFailed((const oscarEvent::ConnectFailed*)e.get()));
    case systemEventEnum::startService:
        return on_startService((const systemEvent::startService*)e.get());
    case rpcEventEnum::PassPacket:
        return(this->on_PassPacket((const rpcEvent::PassPacket*)e.get()));
    case rpcEventEnum::SendPacket:
        return(this->on_SendPacket((const rpcEvent::SendPacket*)e.get()));
    case rpcEventEnum::SubscribeNotifications:
        return(this->on_SubscribeNotifications((const rpcEvent::SubscribeNotifications*)e.get()));
    case rpcEventEnum::UnsubscribeNotifications:
        return(this->on_UnsubscribeNotifications((const rpcEvent::UnsubscribeNotifications*)e.get()));
    case rpcEventEnum::DoListen:
        return(this->DoListen((const rpcEvent::DoListen*)e.get()));
    case oscarEventEnum::Disconnected:
        return(this->on_Disconnected((const oscarEvent::Disconnected*)e.get()));
    case oscarEventEnum::Disaccepted:
        return(this->on_Disaccepted((const oscarEvent::Disaccepted*)e.get()));
#ifdef WEBDUMP
    case webHandlerEventEnum::RequestIncoming:
        return on_RequestIncoming((webHandlerEvent::RequestIncoming*)e.get());
#endif
    }
    XPASS;
    return false;

}


#ifdef WEBDUMP
bool RPC::Service::on_RequestIncoming(const webHandlerEvent::RequestIncoming*e)
{
    HTTP::Response cc(e->req);
    std::string out="<h1>RPC report</h1><p>";

    cc.make_response(out);
    return true;

}
#endif
