#include "referrerClientService.h"
#include "Events/DFS/capsEvent.h"
#include "logging.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#ifndef _WIN32
#include <sys/socket.h>
#include <arpa/inet.h>
#endif
#include "IInstance.h"

#include <algorithm>

#include "route_t.h"

#include "mutexInspector.h"


#include "tools_mt.h"
#include <random>



#include "tools_mt.h"




#include "Events/System/Net/rpcEvent.h"
#include "Events/DFS/referrerEvent.h"
#include <algorithm>
#include "colorOutput.h"

#include "events_referrerClient.hpp"

using namespace referrerClient;
using namespace dfsReferrerEvent;




bool  Service::on_startService(const systemEvent::startService* )
{
    MUTEX_INSPECTOR;

    S_LOG("on_startService");

    sendEvent(ServiceEnum::RPC,new rpcEvent::SubscribeNotifications(ListenerBase::serviceId));
    if(iUtils->isServiceRegistered(ServiceEnum::Telnet))
    {
//        sendEvent(ServiceEnum::Telnet, new telnetEvent::RegisterCommand("services/referrer", "dumpFiles", "show files info","", ListenerBase::serviceId));
//        sendEvent(ServiceEnum::Telnet, new telnetEvent::RegisterCommand("services/referrer", "dumpUsers", "show users info","", ListenerBase::serviceId));
//        sendEvent(ServiceEnum::Telnet, new telnetEvent::RegisterCommand("services/referrer", "dumpPipes", "show pipes info","", ListenerBase::serviceId));
    }

#ifdef WEBDUMP
    {
        sendEvent(ServiceEnum::WebHandler, new webHandlerEvent::RegisterDirectory("dfs","DFS"));
        sendEvent(ServiceEnum::WebHandler, new webHandlerEvent::RegisterHandler("dfs/referrer","Referrer",ListenerBase::serviceId));
    }
#endif

    return true;
}

Service::~Service()
{
}



UnknownBase* Service::construct(const SERVICE_id& id, const std::string&  nm,IInstance* ifa)
{
    XTRY;
    return new Service(id,nm,ifa);
    XPASS;
}

void registerReferrerClientService(const char* pn)
{
    XTRY;

    if(pn)
    {
        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_SERVICE,ServiceEnum::ReferrerClient,"ReferrerClient",getEvents_referrerClient());
    }
    else
    {
        iUtils->registerService(ServiceEnum::ReferrerClient,Service::construct,"ReferrerClient");
        regEvents_referrerClient();
    }
    XPASS;

}



bool Service::on_ConnectFailed(rpcEvent::ConnectFailed const* e)
{
    MUTEX_INSPECTOR;
    S_LOG("on_ConnectFailed");
    /**
      Для центрального сервера никаких реконнектов нет
      */

    msockaddr_in destination_addr=e->destination_addr;
    DBG(logErr2("DFSREferrer::Service::on_ConnectFailed %s (%d)",destination_addr.dump().c_str(),__LINE__));

    d4_on_connect_failed(e->destination_addr);
    d6_on_connect_failed(e->destination_addr);
    return true;
}
bool Service::on_Disconnected(const rpcEvent::Disconnected* e)
{
    MUTEX_INSPECTOR;
    S_LOG("on_Disconnected");

    msockaddr_in addr=e->destination_addr;
    XTRY;

    d4_on_disconnected(e->destination_addr);
    d6_on_disconnected(e->destination_addr);


    XPASS;
    return true;
}


bool  Service::on_Connected(const rpcEvent::Connected* )
{
    return true;
}

bool Service::on_ToplinkDeliverRSP(const dfsReferrerEvent::ToplinkDeliverRSP *e)
{
    MUTEX_INSPECTOR;
    S_LOG("on_ToplinkDeliverRSP");
    if(e->route.size())
    {
        passEvent(e);
    }
    else
    {
        REF_getter<Event::Base> e_extr=e->getEvent();
        if(!e_extr.valid())
            return false;
        passEvent(e_extr);
    }
    return true;

}


bool Service::on_ToplinkDeliverREQ(const dfsReferrerEvent::ToplinkDeliverREQ *e111)
{
    MUTEX_INSPECTOR;
    S_LOG("on_ToplinkDeliverREQ");


    REF_getter<dfsReferrerEvent::ToplinkDeliverREQ> ee=(dfsReferrerEvent::ToplinkDeliverREQ *)e111;



    {
        /// from local service
        if(ee->uuid.size())
            throw CommonError("if(ee->uuid.size()) "+_DMI());

        ee->uuid.insert(iInstance->globalCookie());
    }

    if(hasUL())
    {
        sendToplinkReqClient(getUL(),ee.get());
    }
    else
    {
        throw CommonError("!if(hasUL())");
    }
    return true;

}


bool  Service::on_IncomingOnConnector(const rpcEvent::IncomingOnConnector *evt)
{
    MUTEX_INSPECTOR;
    S_LOG("OnConnector "+evt->esi->remote_name.dump());

    auto& IDC=evt->e->id;
    if( dfsReferrerEventEnum::ToplinkDeliverRSP==IDC)
        return on_ToplinkDeliverRSP(static_cast<const dfsReferrerEvent::ToplinkDeliverRSP* > (evt->e.get()));
    if( dfsReferrerEventEnum::Pong==IDC) /// connector
        return on_Pong(static_cast<const dfsReferrerEvent::Pong* > (evt->e.get()),evt->esi);
    if(dfsReferrerEventEnum::UpdateConfigRSP==IDC)
    {
        const dfsReferrerEvent::UpdateConfigRSP*xe=(const dfsReferrerEvent::UpdateConfigRSP*)evt->e.get();
        config_bod=xe->bod;

        for(auto& z: m_readyNotificationBackroutes)
        {
            passEvent(new dfsReferrerEvent::UpdateConfigRSP(config_bod,poppedFrontRoute(z)));
        }
        return true;
    }

    XTRY;
    MUTEX_INSPECTOR;


    logErr2("unhandled event t t %s %d",iUtils->genum_name(evt->e->id),__LINE__);
    XPASS;
    return false;
}



bool  Service::on_SubscribeNotifications(const dfsReferrerEvent::SubscribeNotifications*ev)
{
    MUTEX_INSPECTOR;
    m_readyNotificationBackroutes.insert(ev->route);
    if(uplinkConnectionState.valid())
    {
        if(uplinkConnectionState->uplink.size())
        {
            passEvent(new dfsReferrerEvent::NotifyReferrerUplinkIsConnected(getUL(),poppedFrontRoute(ev->route)));
            if(config_bod.size())
            {
                passEvent(new dfsReferrerEvent::UpdateConfigRSP(config_bod,poppedFrontRoute(ev->route)));
            }

        }
    }
    return true;

}

bool Service::handleEvent(const REF_getter<Event::Base>& ev)
{
    MUTEX_INSPECTOR;
    S_LOG(__PRETTY_FUNCTION__);
    auto& ID=ev->id;
    XTRY;
    if( systemEventEnum::startService==ID)
        return on_startService((const systemEvent::startService*)ev.get());
    if( timerEventEnum::TickTimer==ID)
        return on_TickTimer((const timerEvent::TickTimer*)ev.get());
    if( timerEventEnum::TickAlarm==ID)
        return on_TickAlarm((const timerEvent::TickAlarm*)ev.get());
#ifdef WEBDUMP
    if( webHandlerEventEnum::RequestIncoming==ID)
        return(this->on_RequestIncoming((const webHandlerEvent::RequestIncoming*)ev.get()));
#endif
    if( telnetEventEnum::CommandEntered==ID)
        return on_CommandEntered((const telnetEvent::CommandEntered*)ev.get());

    if(dfsReferrerEventEnum::UpdateConfigRSP==ID)
    {
        const dfsReferrerEvent::UpdateConfigRSP*xe=(const dfsReferrerEvent::UpdateConfigRSP*)ev.get();
        config_bod=xe->bod;
        return true;
    }

    if(dfsCapsEventEnum::GetReferrersRSP==ID)
    {
        S_LOG("dfsCapsEventEnum::GetReferrersRSP==ID");
        if(stage!=STAGE_D3_GET_GEFERRERS)
        {
            DBG(logErr2("if(stage!=STAGE_D3_GET_GEFERRERS)"));
            return true;
        }

        const dfsCapsEvent::GetReferrersRSP *e=(const dfsCapsEvent::GetReferrersRSP *)ev.get();

        _stop_alarm(T_002_D3_caps_get_service_request_is_timed_out);




        reset_T_001_common_connect_failed();

        int64_t now=iUtils->getNow();
        if(e->referrer_addresses.size()==0)
        {
            for(auto &x: uplinkConnectionState->ponged_all)
            {
                for(auto& y: x.second)
                {
                    {
                        d4_uplink_mode(y.first,y.second);
                        return true;
                    }
                }

            }
            for(auto &x: uplinkConnectionState->ponged_all)
            {
                for(auto& y: x.second)
                {
                    {
                        d4_uplink_mode(y.first,y.second);
                        return true;
                    }
                }

            }

        }
        else
        {
            std::vector<msockaddr_in> sas;
            std::random_device rd;
            std::mt19937 g(rd());
            //std::shuffle(v.begin(), v.end(), g);

            std::shuffle(sas.begin(),sas.end(),g);
            for(auto i:sas)
            {
                sendEvent(i,ServiceEnum::DFSReferrer,
                          new dfsReferrerEvent::Ping(dfsReferrer::PingType::PT_CACHED, iInstance->globalCookie(),
                                                     now,
                                                     connection_sequence_id,
                                                     dfsReferrerEvent::Ping::CT_CLIENT,
                                                     ListenerBase::serviceId));
            }
        }
        stage=STAGE_D3_1_SEND_PING_TO_RESPONDED_REFERRERS;
        _reset_alarm(T_020_D31_wait_after_send_PT_CACHE_on_recvd_from_GetReferrers);



        return true;
    }
    if( rpcEventEnum::Disconnected==ID)
        return(this->on_Disconnected(static_cast<const rpcEvent::Disconnected*>(ev.get())));
    if( rpcEventEnum::ConnectFailed==ID)
        return(this->on_ConnectFailed(static_cast<const rpcEvent::ConnectFailed*>(ev.get())));
    if( rpcEventEnum::Connected==ID)
        return(this->on_Connected(static_cast<const rpcEvent::Connected*>(ev.get())));
    if( rpcEventEnum::IncomingOnConnector==ID)
        return(this->on_IncomingOnConnector(static_cast<const rpcEvent::IncomingOnConnector*>(ev.get())));
    if( dfsReferrerEventEnum::SubscribeNotifications==ID)
        return on_SubscribeNotifications(static_cast<const dfsReferrerEvent::SubscribeNotifications*>(ev.get()));
    if( dfsReferrerEventEnum::ToplinkDeliverRSP==ID)
        return on_ToplinkDeliverRSP(static_cast<const dfsReferrerEvent::ToplinkDeliverRSP* > (ev.get()));
    if( dfsReferrerEventEnum::ToplinkDeliverREQ==ID)       /// forwading
        return on_ToplinkDeliverREQ(static_cast<const dfsReferrerEvent::ToplinkDeliverREQ* > (ev.get()));
    if( dfsReferrerEventEnum::InitClient==ID)       /// forwading
        return on_InitClient(static_cast<const dfsReferrerEvent::InitClient* > (ev.get()));

    logErr2("unhandled event t t %s %d",iUtils->genum_name(ev->id),__LINE__);
    XPASS;
    return false;
}

bool Service::on_CommandEntered(const telnetEvent::CommandEntered*e)
{
    MUTEX_INSPECTOR;
    // if(e->tokens.size())
    {
    }
    return true;
}



bool Service::on_TickTimer(const timerEvent::TickTimer*e)
{
    switch(e->tid)
    {
    default:
        break;
    }
    return false;
}

bool Service::on_TickAlarm(const timerEvent::TickAlarm* e)
{
    MUTEX_INSPECTOR;
    S_LOG("on_TickAlarm");
    refbuffer* b=(refbuffer*) e->cookie.get();
    inBuffer cookieBuf(b);
    int connsecid= static_cast<int>(cookieBuf.get_PN());
    if(connsecid!=connection_sequence_id)
    {
        DBG(logErr2("if(connsecid!=connection_sequence_id) tid %d @@ %s",e->tid,__PRETTY_FUNCTION__));
        return true;
    }
    switch(e->tid)
    {
    case crefTimer::T_020_D31_wait_after_send_PT_CACHE_on_recvd_from_GetReferrers:
    {
        S_LOG("T_020_D31_wait_after_send_PT_CACHE_on_recvd_from_GetReferrers:");
        for(auto &x: uplinkConnectionState->ponged_all)
        {
            for(auto& y: x.second)
            {
                {
                    d4_uplink_mode(y.first,y.second);
                    return true;
                }
            }

        }
        for(auto &x: uplinkConnectionState->ponged_all)
        {
            for(auto& y: x.second)
            {
                {
                    d4_uplink_mode(y.first,y.second);
                    return true;
                }
            }

        }

        reset_T_001_common_connect_failed();
    }
    return true;
    case T_001_common_connect_failed:
    {
        S_LOG("case T_001_common_connect_failed:");
        STAGE_D2_PING_NEIGHBOURS_start();
        return true;
    }



    case T_040_D2_cache_pong_timed_out_from_neighbours:
    {
        S_LOG("T_040_D2_cache_pong_timed_out_from_neighbours");
        if(stage!=STAGE_D2_PING_NEIGHBOURS)
        {
            DBG(logErr2("if(stage!=STAGE_D2_PING_NEIGHBOURS)"));
            return true;
        }
        {
            STAGE_D21_PING_CAPS_start();
        }
        return true;
    }
    case T_002_D3_caps_get_service_request_is_timed_out:
    {
        S_LOG("T_002_D3_caps_get_service_request_is_timed_out");
        if(stage==STAGE_D3_GET_GEFERRERS)
        {
            for(auto &x: uplinkConnectionState->ponged_all)
            {
                for(auto& y: x.second)
                {
//                    if(!isTopServer(y.second))
                    {
                        d4_uplink_mode(y.first,y.second);
                        return true;
                    }
                }

            }
            for(auto &x: uplinkConnectionState->ponged_all)
            {
                for(auto& y: x.second)
                {
//                    if(isTopServer(y.second))
                    {
                        d4_uplink_mode(y.first,y.second);
                        return true;
                    }
                }

            }

        }
        return true;
    }
    case T_007_D6_resend_ping_caps_short:
    case T_008_D6_resend_ping_caps_long:
    case T_009_pong_timed_out_caps_long:
    {
        msockaddr_in remote_addr;
        inBuffer in(e->data);
        in>>remote_addr;
        switch(e->tid)
        {

        case T_007_D6_resend_ping_caps_short:
        {
            S_LOG("T_007_D6_resend_ping_caps_short");
            sendEvent(remote_addr,ServiceEnum::DFSReferrer,
                      new dfsReferrerEvent::Ping(dfsReferrer::PingType::PT_MASTER_SHORT,
                                                 iInstance->globalCookie(),
                                                 iUtils->getNow(),
                                                 connection_sequence_id,
                                                 dfsReferrerEvent::Ping::CT_CLIENT,
                                                 ListenerBase::serviceId));
            _reset_alarm(T_007_D6_resend_ping_caps_short,remote_addr);

        }
        return true;
        case T_008_D6_resend_ping_caps_long:
        {
            S_LOG("T_008_D6_resend_ping_caps_long");
            if(stage==STAGE_D6_MAINTAIN_CONNECTION)
            {
                d6_on_T_011_resend_ping_PT_CAPS_LONG(remote_addr);

            }
            else
            {
                DBG(logErr2("invalid stage"));
            }
        }

        return true;
        case T_009_pong_timed_out_caps_long:
        {
            S_LOG("T_009_pong_timed_out_caps_long");

            d6_on_T_012_pong_timed_out_PT_CAPS_LONG(remote_addr);
        }
        return true;
        default:
            logErr2("invalid timer %d %d",e->tid,__LINE__);
        }
    }
    return true;
    default:
        logErr2("unhandled timer %d %d",e->tid,__LINE__);

    }
    return false;
}

#ifdef WEBDUMP
bool Service::on_RequestIncoming(const webHandlerEvent::RequestIncoming* e)
{
    MUTEX_INSPECTOR;
    HTTP::Response cc(e->req);
    cc.make_response("<h1>Referrer client report</h1><p>");
    return true;

}
#endif
msockaddr_in Service::getUL()
{
    MUTEX_INSPECTOR;
    if(!uplinkConnectionState.valid())
        throw CommonError("if(!uplinkConnectionState.valid())");
    if(uplinkConnectionState->uplink.size()==0)
        throw CommonError("if(uplinkConnectionState->uplink.size()==0) %s",_DMI().c_str());
    return *uplinkConnectionState->uplink.begin();

}
bool Service::hasUL()
{
    MUTEX_INSPECTOR;

    if(!uplinkConnectionState.valid())
        return false;
    if(uplinkConnectionState->uplink.size())
        return true;

    return false;

}



void Service::_reset_alarm(int alarm_id)
{
    MUTEX_INSPECTOR;
    outBuffer o;
    o<<connection_sequence_id;
    resetAlarm(alarm_id,NULL,o.asString().get(),ListenerBase::serviceId);
}
void Service::_reset_alarm(int alarm_id, const msockaddr_in& sa)
{
    MUTEX_INSPECTOR;
    outBuffer o;
    o<<connection_sequence_id;
    resetAlarm(alarm_id,toRef(sa.asString()),o.asString().get(),ListenerBase::serviceId);
}
void Service::_reset_alarm(int alarm_id1,int alarm_id2, const msockaddr_in& sa)
{
    _reset_alarm(alarm_id1,sa);
    _reset_alarm(alarm_id2,sa);
}

void Service::_stop_alarm(int alarm_id)
{
    MUTEX_INSPECTOR;
    stopAlarm(alarm_id,NULL,ListenerBase::serviceId);
}
void Service::_stop_alarm(int alarm_id, const msockaddr_in& sa)
{
    MUTEX_INSPECTOR;
    stopAlarm(alarm_id,toRef(sa.asString()),ListenerBase::serviceId);
}
void Service::_stop_alarm(int alarm_id1, int alarm_id2, const msockaddr_in& sa)
{
    MUTEX_INSPECTOR;
    _stop_alarm(alarm_id1,sa);
    _stop_alarm(alarm_id2,sa);
}
void Service::_stop_alarm(int alarm_id1, int alarm_id2, int alarm_id3, const msockaddr_in& sa)
{
    MUTEX_INSPECTOR;
    _stop_alarm(alarm_id1,sa);
    _stop_alarm(alarm_id2,sa);
    _stop_alarm(alarm_id3,sa);
}

void Service::sendToplinkReqClient(const msockaddr_in& uplink,dfsReferrerEvent::ToplinkDeliverREQ *ee)
{
    MUTEX_INSPECTOR;
    S_LOG("sendToplinkReq");
    {
        if(ee->uuid.size()==0)
        {

        }
        ee->uuid.insert(iInstance->globalCookie());
        sendEvent(uplink,ServiceEnum::DFSReferrer,ee);
    }

}

Service::Service(const SERVICE_id &svs, const std::string &nm, IInstance *ifa):
    UnknownBase(nm),
    ListenerBuffered1Thread(nm,svs),Broadcaster(ifa),
    TimerHelper(ifa),

    uplinkConnectionState(NULL),
    d2_start_time(0),
    iInstance(ifa),
    connection_sequence_id(0),
    neighbours(ifa->getName())


{
    XTRY;

    MUTEX_INSPECTOR;
    try
    {
        XTRY;
        IConfigObj* config=ifa->getConfig();
        setTimerValue(crefTimer::T_001_common_connect_failed,config->get_real("T_001_common_connect_failed", 20,""));
        setTimerValue(crefTimer::T_002_D3_caps_get_service_request_is_timed_out,config->get_real("T_002_D3_caps_get_service_request_is_timed_out", 15.0,""));
        setTimerValue(crefTimer::T_007_D6_resend_ping_caps_short,config->get_real("T_007_D6_resend_ping_caps_short", 7.0,""));
        setTimerValue(crefTimer::T_008_D6_resend_ping_caps_long,config->get_real("T_008_D6_resend_ping_caps_long", 20.0,""));
        setTimerValue(crefTimer::T_009_pong_timed_out_caps_long,config->get_real("T_009_pong_timed_out_caps_long", 40.0,""));
        setTimerValue(crefTimer::T_020_D31_wait_after_send_PT_CACHE_on_recvd_from_GetReferrers,config->get_real("T_020_D31_wait_after_send_PT_CACHE_on_recvd_from_GetReferrers", 2.0,""));
        setTimerValue(crefTimer::T_040_D2_cache_pong_timed_out_from_neighbours,config->get_real("T_040_D2_cache_pong_timed_out_from_neighbours", 2,""));
        XPASS;
    }
    catch(const std::exception &e)
    {
        logErr2("exception: %s %s %d",e.what(),__FILE__,__LINE__);
        throw;
    }
    XPASS;
}

bool Service::d6_on_pong_PT_CAPS_LONG(const msockaddr_in& visible_name_of_pinger, const REF_getter<epoll_socket_info>& esi)
{
    (void)visible_name_of_pinger;
    MUTEX_INSPECTOR;
    S_LOG("d6_on_pong_PT_CAPS_LONG");
    DBG(logErr2("d6_on_pong_PT_CAPS_LONG %s",esi->remote_name().dump().c_str()));
    _reset_alarm(T_008_D6_resend_ping_caps_long,T_009_pong_timed_out_caps_long,esi->remote_name());
    return true;
}
void Service::d6_on_T_011_resend_ping_PT_CAPS_LONG(const msockaddr_in& remote_addr)
{
    S_LOG("d6_on_T_008_D6_resend_ping_caps_long");
    MUTEX_INSPECTOR;
    DBG(logErr2("send PT_CAPS_LONG %s @%s",remote_addr.dump().c_str(),__PRETTY_FUNCTION__));

    sendEvent(remote_addr,ServiceEnum::DFSReferrer,
              new dfsReferrerEvent::Ping(dfsReferrer::PingType::PT_MASTER_LONG,
                                         iInstance->globalCookie(),
                                         iUtils->getNow(),
                                         connection_sequence_id,
                                         dfsReferrerEvent::Ping::CT_CLIENT,
                                         ListenerBase::serviceId));
}
void Service::d6_on_T_012_pong_timed_out_PT_CAPS_LONG(const msockaddr_in& remote_addr)
{
    S_LOG("d6_on_T_009_pong_timed_out_caps_long");
    MUTEX_INSPECTOR;
    d6_on_disconnected(remote_addr);
}
void Service::d6_on_disconnected(const msockaddr_in& remote_addr)
{
    S_LOG("d6_on_disconnected");
    MUTEX_INSPECTOR;
    if(uplinkConnectionState->uplink.count(remote_addr))
    {
        DBG(logErr2("Caps connect broken, try to reconnect..."));

        DBG(logErr2("%s ",remote_addr.dump().c_str()));

        _stop_alarm(T_007_D6_resend_ping_caps_short,T_008_D6_resend_ping_caps_long,T_009_pong_timed_out_caps_long,remote_addr);
        STAGE_D2_PING_NEIGHBOURS_start();
    }
}
void Service::d6_on_connect_failed(const msockaddr_in& remote_addr)
{
    S_LOG("d6_on_connect_failed");
    d6_on_disconnected(remote_addr);
}



#define IGNORE_NAT
bool Service::on_Pong(const dfsReferrerEvent::Pong* e, const REF_getter<epoll_socket_info>& esi)
{
    MUTEX_INSPECTOR;
    S_LOG("on_Pong");
    if(e->globalCookieOfResponder==iInstance->globalCookie())
    {
        DBG(logErr2("if(e->globalCookieOfResponder==iInstance->globalCookie())"));
        return true;
    }
    if(e->connection_sequence_id!=connection_sequence_id)
    {
        DBG(logErr2("if(e->connection_sequence_id!=connection_sequence_id) %d %d",e->connection_sequence_id,connection_sequence_id));
        return true;
    }

    msockaddr_in visible_name_of_pinger=e->visible_name_of_pinger;


    uplinkConnectionState->nodeLevelInHierarhy=e->nodeLevelInHierarhy+1;
    uplinkConnectionState->ponged_for_cleanup_sockets.insert(esi);
    uplinkConnectionState->ponged_all[iUtils->getNow()-e->ping_time].insert(std::make_pair(esi,e->visible_name_of_pinger));
    neighbours.add(esi->remote_name(),e->ping_time);
#ifndef IGNORE_NAT
    if(sa.isNAT())
    {
        DBG(logErr2("isNAT %s",sa.dump().c_str()));
    }
    else
#endif
    {
        if(!iInstance->myExternalAddressesCount(visible_name_of_pinger))
        {
            iInstance->myExternalAddressesInsert(visible_name_of_pinger);
        }
    }

    switch(e->pingType)
    {
    case dfsReferrer::PingType::PT_CACHED:
    {
        MUTEX_INSPECTOR;
        if(stage==STAGE_D2_PING_NEIGHBOURS||stage==STAGE_D21_PING_CAPS)
        {
            bool ok_send=false;

            if(stage==STAGE_D2_PING_NEIGHBOURS)
            {
                {
                    ok_send=true;
                }
            }
            if(stage==STAGE_D21_PING_CAPS)
            {
                {
                    ok_send=true;
                }
            }
            if(ok_send)
            {
                STAGE_D3_GET_GEFERRERS_start(esi->remote_name());
            }
        }
        if(stage==STAGE_D3_1_SEND_PING_TO_RESPONDED_REFERRERS)
        {

            MUTEX_INSPECTOR;
            for(auto& x: uplinkConnectionState->ponged_all)
            {
                for(auto& y: x.second)
                {
                    {
                        if(y.second.getStringAddr()!=e->visible_name_of_pinger.getStringAddr())
                            throw CommonError("if(y.second!=e->visible_name_of_pinger) %s %s",y.second.dump().c_str(),e->visible_name_of_pinger.dump().c_str());
                        _stop_alarm(T_020_D31_wait_after_send_PT_CACHE_on_recvd_from_GetReferrers);
                        d4_uplink_mode(y.first,e->visible_name_of_pinger);
                        return true;
                    }
                }
            }
            for(auto& x: uplinkConnectionState->ponged_all)
            {
                for(auto& y: x.second)
                {
                    {
                        if(y.second.getStringAddr()!=e->visible_name_of_pinger.getStringAddr())
                            throw CommonError("if(y.second!=e->visible_name_of_pinger) %s %s",y.second.dump().c_str(),e->visible_name_of_pinger.dump().c_str());
                        _stop_alarm(T_020_D31_wait_after_send_PT_CACHE_on_recvd_from_GetReferrers);
                        d4_uplink_mode(y.first,e->visible_name_of_pinger);
                        return true;
                    }
                }
            }
        }
        {
            DBG(logErr2("d2_on_pong_PT_CACHED %s",esi->remote_name().dump().c_str()));
        }

        return true;

    }



    case dfsReferrer::PingType::PT_CAPS_SHORT:
    case dfsReferrer::PingType::PT_MASTER_SHORT:
        return d6_on_pong_PT_CAPS_SHORT(e->visible_name_of_pinger, esi);

    case dfsReferrer::PingType::PT_CAPS_LONG:
    case dfsReferrer::PingType::PT_MASTER_LONG:
        return d6_on_pong_PT_CAPS_LONG(e->visible_name_of_pinger, esi);
    default:
        logErr2("invalid pingType %d %s %d",e->pingType,__FILE__,__LINE__);


    }
    return true;

}
bool Service::on_connectionEstablished(const msockaddr_in& remote_addr)
{
    MUTEX_INSPECTOR;
    S_LOG("on_connectionEstablished");
    stopAlarm(crefTimer::T_001_common_connect_failed,NULL,ListenerBase::serviceId);

    DBG(logErr2("bool Service::on_connectionEstablished( ) !!!--- %s",remote_addr.dump().c_str()));

    logErr2(GREEN("referrerClient: connection established %s"),remote_addr.dump().c_str());

    uplinkConnectionState->connectionEstablished=true;

    for(auto &i:m_readyNotificationBackroutes)
    {
        REF_getter<Event::Base> z=new dfsReferrerEvent::NotifyReferrerUplinkIsConnected(remote_addr,poppedFrontRoute(i));
        passEvent(z);
    }
    uplinkConnectionState->uplink.insert(remote_addr);

    logErr2(GREEN("referrerClient: connection established %s"),remote_addr.dump().c_str());

    for(auto &z:uplinkConnectionState->ponged_for_cleanup_sockets)
    {
        if(z->remote_name()!=remote_addr)
        {
            DBG(logErr2("close %s",z->remote_name().dump().c_str()));
            z->close("cleanup unused connections");

        }
    }


    uplinkConnectionState->ponged_for_cleanup_sockets.clear();

    d6_start(remote_addr);
    return true;
}
void Service::d4_on_disconnected(const msockaddr_in& sa)
{
    S_LOG("d4_on_disconnected "+sa.dump());
    MUTEX_INSPECTOR;
    DBG(logErr2("addr is uplink %d",uplinkConnectionState->uplink.count(sa)));
    if(uplinkConnectionState->uplink.count(sa))
    {
        for(auto& z: m_readyNotificationBackroutes)
        {
            passEvent(new dfsReferrerEvent::NotifyReferrerUplinkIsDisconnected(sa,poppedFrontRoute(z)));
        }

        STAGE_D2_PING_NEIGHBOURS_start();
    }
}
void Service::d4_on_connect_failed(const msockaddr_in& sa)
{
    S_LOG("d4_on_connect_failed");
    MUTEX_INSPECTOR;
    DBG(logErr2("d4_on_connect_failed %s",sa.dump().c_str()));
    d4_on_disconnected(sa);

}

void Service::d4_uplink_mode(const REF_getter<epoll_socket_info>& esi, const msockaddr_in &visibleName)
{
    S_LOG("d4_uplink_mode");
    MUTEX_INSPECTOR;
    DBG(logErr2("switch mode"));

    stage=STAGE_D4_MAINTAIN_CONNECTION;

    {
        on_connectionEstablished(esi->remote_name());
    }



}

void Service::d5_start(const REF_getter<epoll_socket_info>&esi,const msockaddr_in& visibleName)
{
    (void)visibleName;
    if(stage==STAGE_D5_CHECK_EXTERNAL)
        return;
    stage=STAGE_D5_CHECK_EXTERNAL;

    S_LOG("d5_start");
    MUTEX_INSPECTOR;
    DBG(logErr2("sa %s ",esi->remote_name().dump().c_str()));


    on_connectionEstablished(esi->remote_name());
}

void Service::d6_start(const msockaddr_in& sa)
{
    stage=STAGE_D6_MAINTAIN_CONNECTION;
    S_LOG("d6_start "+sa.dump());
    MUTEX_INSPECTOR;
    DBG(logErr2("d6_start send PT_CAPS_SHORT"));
    sendEvent(sa,ServiceEnum::DFSReferrer,
              new dfsReferrerEvent::Ping(dfsReferrer::PingType::PT_CAPS_SHORT,
                                         iInstance->globalCookie(),
                                         iUtils->getNow(),
                                         connection_sequence_id,
                                         dfsReferrerEvent::Ping::CT_CLIENT,
                                         ListenerBase::serviceId));
    _reset_alarm(T_007_D6_resend_ping_caps_short,sa);
}
bool Service::d6_on_pong_PT_CAPS_SHORT(const msockaddr_in& visible_name_of_pinger, const REF_getter<epoll_socket_info>& esi)
{
    (void)visible_name_of_pinger;
    MUTEX_INSPECTOR;
    S_LOG("d6_on_pong_PT_CAPS_SHORT");
    {

        _stop_alarm(T_007_D6_resend_ping_caps_short,T_008_D6_resend_ping_caps_long,T_009_pong_timed_out_caps_long,esi->remote_name());
        _reset_alarm(T_008_D6_resend_ping_caps_long,T_009_pong_timed_out_caps_long,esi->remote_name());

    }
    return true;

}
void Service::reset_T_001_common_connect_failed()
{
    outBuffer ocookie;
    ocookie<<connection_sequence_id;
    resetAlarm(crefTimer::T_001_common_connect_failed,NULL,ocookie.asString().get(),ListenerBase::serviceId);
}
void Service::STAGE_D21_PING_CAPS_start()
{
    stage=STAGE_D21_PING_CAPS;
    for(auto &caps:uplinkConnectionState->saCapsFromConfig)
    {
        logErr2("caps %s",caps.dump().c_str());
        auto gcid=iInstance->globalCookie();
        logErr2("sendEvent PT_CACHED %s",caps.dump().c_str());
        sendEvent(caps,ServiceEnum::DFSReferrer,
                  new dfsReferrerEvent::Ping(dfsReferrer::PingType::PT_CACHED, gcid,iUtils->getNow(),
                                             connection_sequence_id,
                                             dfsReferrerEvent::Ping::CT_CLIENT,
                                             ListenerBase::serviceId));
    }
    reset_T_001_common_connect_failed();

}
void Service::STAGE_D2_PING_NEIGHBOURS_start()
{
//    logErr2("STAGE_D2_PING_NEIGHBOURS_start");
    MUTEX_INSPECTOR;
    connection_sequence_id++;

    stage=STAGE_D2_PING_NEIGHBOURS;


    //I_ssl *ssl=(I_ssl*)iUtils->queryIface(Ifaces::SSL);
    if(!uplinkConnectionState.valid())
        throw CommonError("if(!uplinkConnectionState.valid())");
    uplinkConnectionState=new _uplinkConnectionState(uplinkConnectionState->saCapsFromConfig);
    S_LOG("d2_start");
    DBG(logErr2("d2_start"));

    d2_start_time=time(NULL);
    reset_T_001_common_connect_failed();



    std::vector<msockaddr_in> s=neighbours.getAllAndClear();
    if(s.size())
    {
        logErr2("send cached neighbours");
        for(auto& i:s)
        {
//            logErr2("dfsReferrer::PingType::PT_CACHED to %s",i.dump().c_str());
            sendEvent(i,ServiceEnum::DFSReferrer,
                      new dfsReferrerEvent::Ping(dfsReferrer::PingType::PT_CACHED, iInstance->globalCookie(),
                                                 iUtils->getNow(),
                                                 connection_sequence_id,
                                                 dfsReferrerEvent::Ping::CT_CLIENT,
                                                 ListenerBase::serviceId));
        }
        _reset_alarm(T_040_D2_cache_pong_timed_out_from_neighbours);
    }
    else
    {
        STAGE_D21_PING_CAPS_start();
    }

}


void Service::STAGE_D3_GET_GEFERRERS_start(const msockaddr_in& sa)
{
    MUTEX_INSPECTOR;
    stage=STAGE_D3_GET_GEFERRERS;

    S_LOG("d3_start "+sa.dump());
    DBG(logErr2("d3_start"));



    {
        std::set<msockaddr_in> sas=iInstance->myExternalAddressesGet();
        if(sas.size()==0)
            throw CommonError("if(sas.size()==0)");

        DBG(logErr2("dfsCapsEvent::GetReferrersREQ "));

        REF_getter<Event::Base> ze=new dfsCapsEvent::GetReferrersREQ(sas,ListenerBase::serviceId);
        sendToplinkReqClient(sa,new ToplinkDeliverREQ(ServiceEnum::DFSCaps,ze.get(),ListenerBase::serviceId));
        _reset_alarm(T_002_D3_caps_get_service_request_is_timed_out);
    }
    reset_T_001_common_connect_failed();

}

bool Service::on_InitClient(const dfsReferrerEvent::InitClient *e)
{
    MUTEX_INSPECTOR;
    uplinkConnectionState=new _uplinkConnectionState(e->caps);
    STAGE_D2_PING_NEIGHBOURS_start();
    sendEvent(ServiceEnum::RPC,new rpcEvent::SubscribeNotifications(ListenerBase::serviceId));
    return true;
}
