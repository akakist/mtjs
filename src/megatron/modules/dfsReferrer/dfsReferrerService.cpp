#include "dfsReferrerService.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#ifndef _WIN32
#include <sys/socket.h>
#include <arpa/inet.h>
#include <megatron_config.h>
#include <configDB.h>
#include <logging.h>
#include <Events/System/Net/rpcEvent.h>
#include <Events/Tools/webHandlerEvent.h>
#include <version_mega.h>
#include <tools_mt.h>
#include <Events/DFS/referrerEvent.h>
#include <Events/DFS/capsEvent.h>

#endif
#include "Events/Tools/telnetEvent.h"
#include "colorOutput.h"
#include "events_dfsReferrer.hpp"


dfsReferrer::Service::Service(const SERVICE_id &svs, const std::string&  nm, IInstance* ifa):
    UnknownBase(nm),
    ListenerBuffered1Thread(nm,svs),
    Broadcaster(ifa),
    TimerHelper(ifa)

    , iInstance(ifa),rd(ifa)
{

    MUTEX_INSPECTOR;
    try
    {

        XTRY;

        bool iscaps=ifa->getConfig()->get_bool("IsCapsServer",false,
                                               "Is caps mode. Do not forward caps requests to uplink");
        if(!iscaps)
        {
            std::vector<std::string> hline;
            {
                for(int j=0; j<5; j++)
                {
                    char s[100];
                    int port=atoi(CAPS_PORT);
                    snprintf(s,sizeof(s),CAPS_IP ":%d",port+j);
                    hline.push_back(s);
                }
            }
            rd.uplinkConnectionState=new _uplinkConnectionState(
                false,
                ifa->getConfig()->get_tcpaddr("CapsServerUrl",iUtils->join(",",hline),""));
        }
        else
        {
            rd.uplinkConnectionState=new _uplinkConnectionState(true);
        }







        IConfigObj* config=ifa->getConfig();
        setTimerValue(refTimer::T_001_common_connect_failed,config->get_real("T_001_common_connect_failed", 20,""));
        setTimerValue(refTimer::T_002_D3_caps_get_service_request_is_timed_out,config->get_real("T_002_D3_caps_get_service_request_is_timed_out", 15.0,""));
        setTimerValue(refTimer::T_007_D6_resend_ping_caps_short,config->get_real("T_007_D6_resend_ping_caps_short", 7.0,""));
        setTimerValue(refTimer::T_008_D6_resend_ping_caps_long,config->get_real("T_008_D6_resend_ping_caps_long", 20.0,""));
        setTimerValue(refTimer::T_009_pong_timed_out_caps_long,config->get_real("T_009_pong_timed_out_caps_long", 40.0,""));
        setTimerValue(refTimer::T_011_downlink_ping_timed_out,config->get_real("T_011_downlink_ping_timed_out", 60, ""));
        setTimerValue(refTimer::T_012_reregister_referrer,config->get_real("T_012_reregister_referrer", 3500, ""));
        setTimerValue(refTimer::T_020_D31_wait_after_send_PT_CACHE_on_recvd_from_GetReferrers,config->get_real("T_020_D3_1_wait_after_send_PT_CACHE_on_recvd_from_GetService", 2.0,""));
        setTimerValue(refTimer::T_040_D2_cache_pong_timed_out_from_neighbours,config->get_real("T_004_cache_pong_timed_out_", 2,""));


        ConfigDB_private c;
        rd.config_bod=c.get_string("refferer_config","");



        XPASS;
    }
    catch(const std::exception &e)
    {
        logErr2("exception: %s %s %d",e.what(),__FILE__,__LINE__);
        throw;
    }
}
bool  dfsReferrer::Service::on_startService(const systemEvent::startService* )
{
    MUTEX_INSPECTOR;

    S_LOG("on_startService");

    sendEvent(ServiceEnum::RPC,new rpcEvent::SubscribeNotifications(ListenerBase::serviceId));
    if(iUtils->isServiceRegistered(ServiceEnum::Telnet))
    {
    }

#ifdef WEBDUMP
    {
        sendEvent(ServiceEnum::WebHandler, new webHandlerEvent::RegisterDirectory("dfs","DFS"));
        sendEvent(ServiceEnum::WebHandler, new webHandlerEvent::RegisterHandler("dfs/referrer","Referrer",ListenerBase::serviceId));
    }
#endif



    if(!rd.uplinkConnectionState->m_isTopServer)
    {
        STAGE_D2_PING_NEIGHBOURS_start();
        return true;
    }
    return true;
}

dfsReferrer::Service::~Service()
    = default;



UnknownBase* dfsReferrer::Service::construct(const SERVICE_id& id, const std::string&  nm,IInstance* ifa)
{
    XTRY;
    return new Service(id,nm,ifa);
    XPASS;
}

void registerDFSReferrerService(const char* pn)
{
    XTRY;
    if(pn)
    {
        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_SERVICE,ServiceEnum::DFSReferrer,"DFSReferrer",getEvents_dfsReferrer());
    }
    else
    {
        iUtils->registerService(ServiceEnum::DFSReferrer,dfsReferrer::Service::construct,"DFSReferrer");
        regEvents_dfsReferrer();
    }
    XPASS;

}


bool dfsReferrer::Service::on_ConnectFailed(rpcEvent::ConnectFailed const* e)
{
    MUTEX_INSPECTOR;
    S_LOG("on_ConnectFailed");
    /**
      Для центрального сервера никаких реконнектов нет
      */

    DBG(logErr2("DFSREferrer::Service::on_ConnectFailed %s (%s %d)",e->destination_addr.dump().c_str(),__FILE__,__LINE__));

    d4_on_connect_failed(e->destination_addr);
    d6_on_connect_failed(e->destination_addr);


    return true;
}
bool dfsReferrer::Service::on_Disconnected(const rpcEvent::Disconnected* e)
{
    MUTEX_INSPECTOR;
    S_LOG("on_Disconnected");

    XTRY;

    d4_on_disconnected(e->destination_addr);
    d6_on_disconnected(e->destination_addr);

    XPASS;
    return true;
}
void dfsReferrer::Service::on_deleteDownlink(const SOCKET_id& sock, const msockaddr_in& )
{
    MUTEX_INSPECTOR;

    REF_getter<linkInfoDownReferrer> di=rd.urirefs->getDownlinkOrNull(sock);
    {
        MUTEX_INSPECTOR;
        if(di.valid())
        {
            DBG(logErr2("disaccepted valid user %s, do smth for notify",di->esi_->remote_name().dump().c_str()));
        }

        if(di.valid())
        {
            DBG(logErr2("urirefs->downlinks.erase %d", CONTAINER(sock)));
            rd.urirefs->eraseFromDownlinks(sock);
            for(auto& z:rd.m_readyNotificationBackroutes)
            {
                passEvent(new dfsReferrerEvent::NotifyReferrerDownlinkDisconnected(sock,poppedFrontRoute(z)));
            }
        }
        else
        {
            DBG(logErr2("invalid di because host is not downlink"));
        }
    }
}
bool dfsReferrer::Service::on_Disaccepted(const rpcEvent::Disaccepted* e)
{
    MUTEX_INSPECTOR;
    /**
      Удаление дочерних данных
     */

    XTRY;
    S_LOG("on_Disaccepted sock "+std::to_string(CONTAINER(e->esi->id_)));
    if(e->esi->remote_name_.has_value())
        on_deleteDownlink(e->esi->id_,*e->esi->remote_name_);
    XPASS;
    return true;
}

bool dfsReferrer::Service::on_Accepted(const rpcEvent::Accepted* )
{
    return true;
}

bool  dfsReferrer::Service::on_Connected(const rpcEvent::Connected* )
{
    return true;
}


bool dfsReferrer::Service::on_ToplinkDeliverRSP(const dfsReferrerEvent::ToplinkDeliverRSP *e)
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


bool dfsReferrer::Service::on_ToplinkDeliverREQ(const dfsReferrerEvent::ToplinkDeliverREQ *e111, const rpcEvent::IncomingOnAcceptor* acc)
{
    MUTEX_INSPECTOR;
    S_LOG("on_ToplinkDeliverREQ");

    REF_getter<dfsReferrerEvent::ToplinkDeliverREQ> e_toplink=(dfsReferrerEvent::ToplinkDeliverREQ *)e111;


    if(rd.uplinkConnectionState->m_isTopServer)
    {
        MUTEX_INSPECTOR;
        S_LOG("m_isTopServer");
        if(e_toplink->destinationService==ServiceEnum::DFSCaps)
        {
            MUTEX_INSPECTOR;
            sendEvent(ServiceEnum::DFSCaps,e_toplink.get());
            return true;
        }
        if(e_toplink->destinationService==ServiceEnum::DFSReferrer)
        {
            MUTEX_INSPECTOR;
            REF_getter<Event::Base> z=e_toplink->getEvent();
            if(!z.valid())
                return false;
            return handleEvent(z);
        }
        sendEvent(e_toplink->destinationService,e_toplink.get());
        return true;
    }

    if(acc)
    {
        if(e_toplink->uuid.count(iInstance->globalCookie()))
        {
            DBG(logErr2("ciclic uuid %s",e_toplink->route.dump().c_str()));
            acc->esi->close("ciclic uuid");
            return true;
        }
        e_toplink->uuid.insert(iInstance->globalCookie());
    }
    else
    {
        /// from local service
        if(e_toplink->uuid.size())
            throw CommonError("if(ee->uuid.size()) "+_DMI());

        e_toplink->uuid.insert(iInstance->globalCookie());
    }

    if(hasUL())
    {
        MUTEX_INSPECTOR;
        sendToplinkReq(getUL(),e_toplink.get());
    }
    else
    {
        throw CommonError("%s !if(hasUL()) %s %d", iInstance->getName().c_str(),__FILE__,__LINE__);
    }
    return true;

}


bool  dfsReferrer::Service::on_IncomingOnConnector(const rpcEvent::IncomingOnConnector *evt)
{
    MUTEX_INSPECTOR;
    S_LOG("OnConnector "+evt->esi->remote_name.dump());

    auto& IDC=evt->e->id;
    if( dfsReferrerEventEnum::ToplinkDeliverRSP==IDC)
        return on_ToplinkDeliverRSP(static_cast<const dfsReferrerEvent::ToplinkDeliverRSP* > (evt->e.get()));
    if( dfsReferrerEventEnum::Pong==IDC) /// connector
        return on_Pong(static_cast<const dfsReferrerEvent::Pong* > (evt->e.get()),evt->esi);
    if( dfsReferrerEventEnum::UpdateConfigRSP==IDC)
        return on_UpdateConfigRSP(static_cast<const dfsReferrerEvent::UpdateConfigRSP* > (evt->e.get()));


    XTRY


    logErr2("unhandled event t t %s %s %d",iUtils->genum_name(evt->e->id),__FILE__,__LINE__);
    XPASS;
    return false;
}

bool  dfsReferrer::Service::on_IncomingOnAcceptor(const rpcEvent::IncomingOnAcceptor*evt)
{
    MUTEX_INSPECTOR;
    S_LOG("OnAcceptor "+evt->esi->remote_name.dump());
    XTRY;
    auto& IDA=evt->e->id;
    if(dfsReferrerEventEnum::Ping==IDA)/// acceptor
    {
        if(rd.urirefs->countInDownlinks(evt->esi->id_))
        {

            resetAlarm(refTimer::T_011_downlink_ping_timed_out,
                       outV(CONTAINER(evt->esi->id_),evt->esi->remote_name()),
                       outV(rd.connection_sequence_id).get(),
                       ListenerBase::serviceId);
        }
    }
    if(dfsReferrerEventEnum::ToplinkDeliverRSP==IDA)
    {
        MUTEX_INSPECTOR;
        passEvent(evt->e);
        return true;
    }
    if( dfsReferrerEventEnum::SubscribeNotifications==IDA)
        return on_SubscribeNotifications((const dfsReferrerEvent::SubscribeNotifications*)evt->e.get());
    if( dfsReferrerEventEnum::ToplinkDeliverREQ==IDA)        /// forwading
        return on_ToplinkDeliverREQ(static_cast<const dfsReferrerEvent::ToplinkDeliverREQ* > (evt->e.get()),evt);
    if( dfsReferrerEventEnum::Ping==IDA)/// acceptor
        return on_Ping(static_cast<const dfsReferrerEvent::Ping* > (evt->e.get()),evt->esi);
    if( dfsReferrerEventEnum::UpdateConfigRSP==IDA)
        return on_UpdateConfigRSP(static_cast<const dfsReferrerEvent::UpdateConfigRSP* > (evt->e.get()));


    logErr2("unhandled event %s %s %d",iUtils->genum_name(evt->e->id),__FILE__,__LINE__);
    XPASS;
    return false;
}


bool  dfsReferrer::Service::on_SubscribeNotifications(const dfsReferrerEvent::SubscribeNotifications*ev)
{
    MUTEX_INSPECTOR;
    rd.m_readyNotificationBackroutes.insert(ev->route);
    if(hasUL())
    {
        passEvent(new dfsReferrerEvent::NotifyReferrerUplinkIsConnected(getUL(),poppedFrontRoute(ev->route)));
    }
    return true;

}


bool dfsReferrer::Service::handleEvent(const REF_getter<Event::Base>& ev)
{
    MUTEX_INSPECTOR;
    S_LOG(iInstance->getName()+" dfsReferrer HE");

    auto& ID=ev->id;
    XTRY;
    switch(ID)
    {
    case systemEventEnum::startService:
        return on_startService((const systemEvent::startService*)ev.get());
    case timerEventEnum::TickTimer:
        return on_TickTimer((const timerEvent::TickTimer*)ev.get());
    case timerEventEnum::TickAlarm:
        return on_TickAlarm((const timerEvent::TickAlarm*)ev.get());
#ifdef WEBDUMP
    case webHandlerEventEnum::RequestIncoming:
        return(this->on_RequestIncoming((const webHandlerEvent::RequestIncoming*)ev.get()));
#endif
    case telnetEventEnum::CommandEntered:
        return on_CommandEntered((const telnetEvent::CommandEntered*)ev.get());

    case dfsCapsEventEnum::GetReferrersRSP:
    {
        S_LOG("dfsCapsEventEnum::GetReferrersRSP==ID");
        if(rd.stage!=_dfsReferrerData::_stage::STAGE_D3_GET_GEFERRERS)
        {
            DBG(logErr2("if(stage!=STAGE_D3_GET_GEFERRERS)"));
            return true;
        }

        const auto *e=(const dfsCapsEvent::GetReferrersRSP *)ev.get();

        _stop_alarm(T_002_D3_caps_get_service_request_is_timed_out);



        reset_T_001_common_connect_failed();

        int64_t now=iUtils->getNow();
        if(e->referrer_addresses.size()==0)
        {
            for(auto &x: rd.uplinkConnectionState->ponged_all)
            {
                for(auto& y: x.second)
                {
                    if(!isTopServer(y.second))
                    {
                        d4_uplink_mode(y.first,y.second);
                        return true;
                    }
                }

            }
            for(auto &x: rd.uplinkConnectionState->ponged_all)
            {
                for(auto& y: x.second)
                {
                    if(isTopServer(y.second))
                    {
                        d4_uplink_mode(y.first,y.second);
                        return true;
                    }
                }

            }
            return true;

        }
        else
        {
            for(const auto& i:e->referrer_addresses)
            {
                sendEvent(i,ServiceEnum::DFSReferrer,
                          new dfsReferrerEvent::Ping(dfsReferrer::PingType::PT_CACHED, iInstance->globalCookie(),
                                                     getRpcExternalListenPortMain(iInstance),
                                                     getRpcInternalListenAddrs(iInstance),
                                                     now,
                                                     rd.connection_sequence_id,dfsReferrerEvent::Ping::CT_NODE,
                                                     ListenerBase::serviceId));
            }
        }
        rd.stage=_dfsReferrerData::_stage::STAGE_D3_1_SEND_PING_TO_RESPONDED_REFERRERS;
        _reset_alarm(T_020_D31_wait_after_send_PT_CACHE_on_recvd_from_GetReferrers);



        return true;
    }
    case rpcEventEnum::Disconnected:
        return(this->on_Disconnected((const rpcEvent::Disconnected*)ev.get()));
    case rpcEventEnum::Disaccepted:
        return this->on_Disaccepted((const rpcEvent::Disaccepted *) ev.get());
    case rpcEventEnum::ConnectFailed:
        return this->on_ConnectFailed((const rpcEvent::ConnectFailed *) ev.get());
    case rpcEventEnum::Connected:
        return this->on_Connected((const rpcEvent::Connected *) ev.get());
    case rpcEventEnum::Accepted:
        return this->on_Accepted((const rpcEvent::Accepted *) ev.get());
    case rpcEventEnum::Binded:
        return this->on_Binded((const rpcEvent::Binded *) ev.get());
    case rpcEventEnum::IncomingOnAcceptor:
        return(this->on_IncomingOnAcceptor((const rpcEvent::IncomingOnAcceptor*)ev.get()));
    case rpcEventEnum::IncomingOnConnector:
        return(this->on_IncomingOnConnector((const rpcEvent::IncomingOnConnector*)ev.get()));
    case dfsReferrerEventEnum::SubscribeNotifications:
        return on_SubscribeNotifications((const dfsReferrerEvent::SubscribeNotifications*)ev.get());
    case dfsReferrerEventEnum::ToplinkDeliverRSP:
        return on_ToplinkDeliverRSP(dynamic_cast<const dfsReferrerEvent::ToplinkDeliverRSP* > (ev.get()));
    case dfsReferrerEventEnum::ToplinkDeliverREQ:       /// forwading
        return on_ToplinkDeliverREQ(dynamic_cast<const dfsReferrerEvent::ToplinkDeliverREQ* > (ev.get()),NULL);

    case dfsReferrerEventEnum::UpdateConfigRSP:
        return on_UpdateConfigRSP(dynamic_cast<const dfsReferrerEvent::UpdateConfigRSP* > (ev.get()));
    case dfsReferrerEventEnum::UpdateConfigREQ:
        return on_UpdateConfigREQ(dynamic_cast<const dfsReferrerEvent::UpdateConfigREQ* > (ev.get()));


    }

    logErr2("unhandled event t t %s %s %d",iUtils->genum_name(ev->id),__FILE__,__LINE__);
    XPASS;
    return false;
}

bool dfsReferrer::Service::on_CommandEntered(const telnetEvent::CommandEntered*e)
{
    MUTEX_INSPECTOR;
    return true;
}



bool dfsReferrer::Service::on_TickTimer(const timerEvent::TickTimer*e)
{
    MUTEX_INSPECTOR;
    (void)e;
//    switch(e->tid)
//    {
//    }
    return false;
}

bool dfsReferrer::Service::on_TickAlarm(const timerEvent::TickAlarm* e)
{
    MUTEX_INSPECTOR;
    S_LOG("on_TickAlarm");
    refbuffer *b=(refbuffer *)e->cookie.get();
    inBuffer cookieBuf(b);
    int connsecid= static_cast<int>(cookieBuf.get_PN());
    if(connsecid!=rd.connection_sequence_id)
    {
        DBG(logErr2("if(connsecid!=connection_sequence_id)"));
        return true;
    }
    switch(e->tid)
    {
    case T_020_D31_wait_after_send_PT_CACHE_on_recvd_from_GetReferrers:
    {
        S_LOG("T_020_D31_wait_after_send_PT_CACHE_on_recvd_from_GetReferrers:");
        for(auto &x: rd.uplinkConnectionState->ponged_all)
        {
            for(auto& y: x.second)
            {
                if(!isTopServer(y.second))
                {
                    d4_uplink_mode(y.first,y.second);
                    return true;
                }
            }

        }
        for(auto &x: rd.uplinkConnectionState->ponged_all)
        {
            for(auto& y: x.second)
            {
                if(isTopServer(y.second))
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


    case T_011_downlink_ping_timed_out:
    {
        MUTEX_INSPECTORS("T_011_downlink_ping_timed_out");
        inBuffer in(e->data);
        SOCKET_id sid;
        CONTAINER(sid)=in.get_PN();
        msockaddr_in sa;
        in>>sa;
        on_deleteDownlink(sid,sa);
        return true;
    }
    case T_012_reregister_referrer:
    {
        S_LOG("T_012_reregister_referrer");
        registerReferrer(getUL());
    }
    return true;
    case T_040_D2_cache_pong_timed_out_from_neighbours:
    {
        S_LOG("T_040_D2_cache_pong_timed_out_from_neighbours");
        if(rd.stage!=_dfsReferrerData::_stage::STAGE_D2_PING_NEIGHBOURS)
        {
            DBG(logErr2("if(stage!=STAGE_D2_PING_NEIGHBOURS)"));
            //STAGE_D2_PING_NEIGHBOURS_start();
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
        if(rd.stage==_dfsReferrerData::_stage::STAGE_D3_GET_GEFERRERS)
        {
            for(auto &x: rd.uplinkConnectionState->ponged_all)
            {
                for(auto& y: x.second)
                {
                    if(!isTopServer(y.second))
                    {
                        d4_uplink_mode(y.first,y.second);
                        return true;
                    }
                }

            }
            for(auto &x: rd.uplinkConnectionState->ponged_all)
            {
                for(auto& y: x.second)
                {
                    if(isTopServer(y.second))
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
                      new dfsReferrerEvent::Ping(isTopServer(remote_addr)?dfsReferrer::PingType::PT_CAPS_SHORT:dfsReferrer::PingType::PT_MASTER_SHORT,
                                                 iInstance->globalCookie(),
                                                 getRpcExternalListenPortMain(iInstance),
                                                 getRpcInternalListenAddrs(iInstance),
                                                 iUtils->getNow(),
                                                 rd.connection_sequence_id,
                                                 dfsReferrerEvent::Ping::CT_NODE,
                                                 ListenerBase::serviceId));
            _reset_alarm(T_007_D6_resend_ping_caps_short,remote_addr);

        }
        return true;
        case T_008_D6_resend_ping_caps_long:
        {
            S_LOG("T_008_D6_resend_ping_caps_long");
            if(rd.stage==_dfsReferrerData::_stage::STAGE_D6_MAINTAIN_CONNECTION)
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
            logErr2("invalid timer %d %s %d",e->tid,__FILE__,__LINE__);
        }
    }
    return true;
    default:
        logErr2("unhandled timer %d %s %d",e->tid,__FILE__,__LINE__);

    }
    return false;
}

#ifdef WEBDUMP
bool dfsReferrer::Service::on_RequestIncoming(const webHandlerEvent::RequestIncoming* e)
{
    MUTEX_INSPECTOR;
    HTTP::Response cc(e->req);
    std::string out="<h1>DFS Referrer report</h1><p>";

    out+=rd.uplinkConnectionState->getWebDumpableLink("uplinkConnectionState")+"<p>";
    out+=rd.urirefs->getWebDumpableLink("urirefs")+"<p>";

    cc.make_response(out);
    return true;

}
#endif

bool dfsReferrer::Service::on_Binded(const rpcEvent::Binded*e)
{
    MUTEX_INSPECTOR;
    if((std::string)e->esi->socketDescription_=="RPC")
    {
    }
    return true;
}
void dfsReferrer::Service::kill_downlinks()
{
    MUTEX_INSPECTOR;
    auto downlinks=rd.urirefs->getDownlinks();
    for(auto &i:downlinks)
    {
        REF_getter<linkInfoDownReferrer> &di=i.second;
        {
            di->esi_->close("uplink disconnected");
        }
    }
}
msockaddr_in dfsReferrer::Service::getUL()
{
    MUTEX_INSPECTOR;
    if(rd.uplinkConnectionState->uplink.empty())
        throw CommonError("if(uplinkConnectionState->uplink.size()==0) %s",_DMI().c_str());
    return *rd.uplinkConnectionState->uplink.begin();

}
bool dfsReferrer::Service::hasUL()
{
    MUTEX_INSPECTOR;
    return !rd.uplinkConnectionState->uplink.empty();

}
void dfsReferrer::Service::registerReferrer(const msockaddr_in& destination)
{
    MUTEX_INSPECTOR;
    S_LOG("void dfsReferrer::Service::registerReferrer(const msockaddr_in&)");
//    if(rd.externalAccessIsPossible)
    {

        DBG(logErr2("send dfsCapsEvent::RegisterMyRefferrerNodeREQ"));
        REF_getter<Event::Base> cr=
            new dfsCapsEvent::RegisterMyRefferrerNodeREQ(iInstance->myExternalAddressesGet(),destination,rd.urirefs->sizeOfDownlinks(), ListenerBase::serviceId);
        sendToplinkReq(destination,new dfsReferrerEvent::ToplinkDeliverREQ(ServiceEnum::DFSCaps,cr.get(),ListenerBase::serviceId));

    }
//    else if(m_enableExternalCheckConnection)
//        throw CommonError("!externalAccessIsPossible");

    resetAlarm(T_012_reregister_referrer,nullptr,outV(rd.connection_sequence_id).get(),ListenerBase::serviceId);

}



void dfsReferrer::Service::_reset_alarm(int alarm_id)
{
    MUTEX_INSPECTOR;
    resetAlarm(alarm_id,nullptr,outV(rd.connection_sequence_id).get(),ListenerBase::serviceId);
}
void dfsReferrer::Service::_reset_alarm(int alarm_id, const msockaddr_in& sa)
{
    MUTEX_INSPECTOR;
    resetAlarm(alarm_id,outV(sa),outV(rd.connection_sequence_id).get(),ListenerBase::serviceId);
}
void dfsReferrer::Service::_reset_alarm(int alarm_id1,int alarm_id2, const msockaddr_in& sa)
{
    MUTEX_INSPECTOR;
    _reset_alarm(alarm_id1,sa);
    _reset_alarm(alarm_id2,sa);
}


void dfsReferrer::Service::_stop_alarm(int alarm_id)
{
    MUTEX_INSPECTOR;
    stopAlarm(alarm_id,nullptr,ListenerBase::serviceId);
}
void dfsReferrer::Service::_stop_alarm(int alarm_id, const msockaddr_in& sa)
{
    MUTEX_INSPECTOR;
    stopAlarm(alarm_id,outV(sa),ListenerBase::serviceId);
}
void dfsReferrer::Service::_stop_alarm(int alarm_id1, int alarm_id2, const msockaddr_in& sa)
{
    MUTEX_INSPECTOR;
    _stop_alarm(alarm_id1,sa);
    _stop_alarm(alarm_id2,sa);
}
void dfsReferrer::Service::_stop_alarm(int alarm_id1, int alarm_id2, int alarm_id3, const msockaddr_in& sa)
{
    MUTEX_INSPECTOR;
    _stop_alarm(alarm_id1,sa);
    _stop_alarm(alarm_id2,sa);
    _stop_alarm(alarm_id3,sa);
}

bool dfsReferrer::Service::on_Ping(const dfsReferrerEvent::Ping* e, const REF_getter<epoll_socket_info>& _esi)
{
    MUTEX_INSPECTOR;
//    logErr2("on_Ping %s",e->dump().toStyledString().c_str());
    S_LOG("on_Ping");
    if(rd.uplinkConnectionState->m_isTopServer && e->clientType==dfsReferrerEvent::Ping::CT_CLIENT)
    {
        logErr2("caps cannot connect client type");
        return true;
    }

    bool valid=rd.uplinkConnectionState->connectionEstablished && !rd.uplinkConnectionState->uplink.empty() ;
    if(rd.uplinkConnectionState->m_isTopServer) valid=true;
    if(!valid)
    {
        /// if no uplink dont reply pong
        _esi->close("no uplink");
        return true;
    }
    if(!CONTAINER(e->globalCookieOfSender).empty())
    {
        if(e->globalCookieOfSender==iInstance->globalCookie())
        {
            DBG(logErr2("on ping globalCookie matched %s",iUtils->genum_name(e->id)));
            _esi->close("same globalCookie");
            return true;
        }
    }
    switch(e->pingType)
    {
    case dfsReferrer::PingType::PT_CAPS_LONG:
    case dfsReferrer::PingType::PT_CAPS_SHORT:
    case dfsReferrer::PingType::PT_MASTER_LONG:
    case dfsReferrer::PingType::PT_MASTER_SHORT:
    {
        /// создаем даунлинк для целей бакруты
        if(_esi.valid())
        {
            msockaddr_in sa=_esi->remote_name();
            sa.setPort(e->externalListenPort);
            bool created=false;
            REF_getter<linkInfoDownReferrer>l=rd.urirefs->getDownlinkOrCreate(_esi,sa,e->internalListenAddr,e->route,&created);
            if(created)
            {
                route_t br;
                {
                    br=l->backRoute_;
                }
                passEvent(new dfsReferrerEvent::UpdateConfigRSP(rd.config_bod,poppedFrontRoute(br)));

            }
        }
    }
    break;
    default:
        break;
    }

    {
        passEvent(new dfsReferrerEvent::Pong(e->pingType,_esi->remote_name(),
                                             iInstance->globalCookie(),
                                             rd.uplinkConnectionState->nodeLevelInHierarhy,
                                             e->ping_time,
                                             e->connection_sequence_id,
                                             poppedFrontRoute(e->route)));
        return true;
    }
    return true;

}
void dfsReferrer::Service::sendToplinkReq(const msockaddr_in& uplink,dfsReferrerEvent::ToplinkDeliverREQ *ee)
{
    MUTEX_INSPECTOR;
    S_LOG("sendToplinkReq");
    //DBG(logErr2("req %s",ee->dump().c_str()));
    if(rd.uplinkConnectionState->m_isTopServer)
    {
        S_LOG("m_isTopServer");
        if(ee->destinationService==ServiceEnum::DFSReferrer)
        {

            sendEvent(ee->destinationService,ee);
        }
        if(ee->destinationService==ServiceEnum::DFSCaps)
        {
            sendEvent(ServiceEnum::DFSCaps,ee);
            return;
        }
        sendEvent(ee->destinationService,ee);
    }
    else
    {
        if(ee->uuid.empty())
        {

        }
        ee->uuid.insert(iInstance->globalCookie());
        sendEvent(uplink,ServiceEnum::DFSReferrer,ee);
    }


}

bool dfsReferrer::Service::d6_on_pong_PT_CAPS_LONG(const REF_getter<epoll_socket_info>& esi)
{
    MUTEX_INSPECTOR;
    S_LOG("d6_on_pong_PT_CAPS_LONG");
    DBG(logErr2("d6_on_pong_PT_CAPS_LONG %s",esi->remote_name().dump().c_str()));

    _reset_alarm(T_008_D6_resend_ping_caps_long,esi->remote_name());
    _reset_alarm(T_009_pong_timed_out_caps_long,esi->remote_name());
    return true;
}
void dfsReferrer::Service::d6_on_T_011_resend_ping_PT_CAPS_LONG(const msockaddr_in& remote_addr)
{
    S_LOG("d6_on_T_008_D6_resend_ping_caps_long");
    MUTEX_INSPECTOR;
//    DBG(logErr2("send PT_CAPS_LONG %s @%s",remote_addr.dump().c_str(),__PRETTY_FUNCTION__));

    sendEvent(remote_addr,ServiceEnum::DFSReferrer,
              new dfsReferrerEvent::Ping(isTopServer(remote_addr)?dfsReferrer::PingType::PT_CAPS_LONG:dfsReferrer::PingType::PT_MASTER_LONG,
                                         iInstance->globalCookie(),
                                         getRpcExternalListenPortMain(iInstance),
                                         getRpcInternalListenAddrs(iInstance),
                                         iUtils->getNow(),
                                         rd.connection_sequence_id,
                                         dfsReferrerEvent::Ping::CT_NODE,
                                         ListenerBase::serviceId));
}
void dfsReferrer::Service::d6_on_T_012_pong_timed_out_PT_CAPS_LONG(const msockaddr_in& remote_addr)
{
    S_LOG("d6_on_T_009_pong_timed_out_caps_long");
    MUTEX_INSPECTOR;
    d6_on_disconnected(remote_addr);
}
void dfsReferrer::Service::d6_on_disconnected(const msockaddr_in& remote_addr)
{
    S_LOG("d6_on_disconnected");
    MUTEX_INSPECTOR;
    if(rd.uplinkConnectionState->uplink.count(remote_addr))
    {
        DBG(logErr2("Caps connect broken, try to reconnect..."));

        DBG(logErr2("%s ",remote_addr.dump().c_str()));

        _stop_alarm(T_007_D6_resend_ping_caps_short,remote_addr);
        _stop_alarm(T_008_D6_resend_ping_caps_long,remote_addr);
        _stop_alarm(T_009_pong_timed_out_caps_long,remote_addr);
        STAGE_D2_PING_NEIGHBOURS_start();
    }
}
void dfsReferrer::Service::d6_on_connect_failed(const msockaddr_in& remote_addr)
{
    MUTEX_INSPECTOR;
    S_LOG("d6_on_connect_failed");
    d6_on_disconnected(remote_addr);
}



#define IGNORE_NAT
bool dfsReferrer::Service::on_Pong(const dfsReferrerEvent::Pong* e, const REF_getter<epoll_socket_info>& esi)
{
    MUTEX_INSPECTOR;
    S_LOG("on_Pong");
    if(e->globalCookieOfResponder==iInstance->globalCookie())
    {
        DBG(logErr2("if(e->globalCookieOfResponder==iInstance->globalCookie())"));
        return true;
    }
    if(e->connection_sequence_id!=rd.connection_sequence_id)
    {
        DBG(logErr2("if(e->connection_sequence_id!=connection_sequence_id) %d %d",e->connection_sequence_id,rd.connection_sequence_id));
        return true;
    }

    msockaddr_in visible_name_of_pinger=e->visible_name_of_pinger;

    visible_name_of_pinger.setPort(getRpcExternalListenPortMain(iInstance));


    rd.uplinkConnectionState->nodeLevelInHierarhy=e->nodeLevelInHierarhy+1;
    rd.uplinkConnectionState->ponged_for_cleanup_sockets.insert(esi);
    rd.uplinkConnectionState->ponged_all[iUtils->getNow()-e->ping_time].insert(std::make_pair(esi,e->visible_name_of_pinger));
    if(rd.uplinkConnectionState->m_isTopServer && rd.uplinkConnectionState->nodeLevelInHierarhy!=0)
    {
        throw CommonError("if(uplinkConnectionState->m_isTopServer && uplinkConnectionState->nodeLevelInHierarhy!=0)");
    }
    rd.neighbours.add(esi->remote_name(),e->ping_time);
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
    case PingType::PT_CACHED:
    {
        if(rd.stage==_dfsReferrerData::_stage::STAGE_D2_PING_NEIGHBOURS||rd.stage==_dfsReferrerData::_stage::STAGE_D21_PING_CAPS)
        {
            bool ok_send=false;

            if(rd.stage==_dfsReferrerData::_stage::STAGE_D2_PING_NEIGHBOURS)
            {
                if(!isTopServer(esi->remote_name()))
                {
                    ok_send=true;
                }
            }
            if(rd.stage==_dfsReferrerData::_stage::STAGE_D21_PING_CAPS)
            {
                if(isTopServer(esi->remote_name()))
                {
                    ok_send=true;
                }
            }
            if(ok_send)
            {
                STAGE_D3_GET_GEFERRERS_start(esi->remote_name());
            }
        }
        if(rd.stage==_dfsReferrerData::_stage::STAGE_D3_1_SEND_PING_TO_RESPONDED_REFERRERS)
        {

            for(auto& x: rd.uplinkConnectionState->ponged_all)
            {
                for(auto& y: x.second)
                {
                    if(!isTopServer(y.first->remote_name()))
                    {
                        if(y.second==e->visible_name_of_pinger)
                        {
//                            throw CommonError("if(y.second!=e->visible_name_of_pinger) %s %s",y.second.dump().c_str(),e->visible_name_of_pinger.dump().c_str());
                            _stop_alarm(T_020_D31_wait_after_send_PT_CACHE_on_recvd_from_GetReferrers);
                            d4_uplink_mode(y.first,e->visible_name_of_pinger);
                            return true;
                        }
                    }
                }
            }
            for(auto& x: rd.uplinkConnectionState->ponged_all)
            {
                for(auto& y: x.second)
                {
                    if(isTopServer(y.first->remote_name()))
                    {
                        if(y.second!=e->visible_name_of_pinger)
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
    case PingType::PT_CAPS_SHORT:
    case PingType::PT_MASTER_SHORT:
        return d6_on_pong_PT_CAPS_SHORT(esi);

    case PingType::PT_CAPS_LONG:
    case PingType::PT_MASTER_LONG:
        return d6_on_pong_PT_CAPS_LONG(esi);
    default:
        logErr2("invalid pingType %d %s %d",e->pingType,__FILE__,__LINE__);


    }
    return true;

}

bool dfsReferrer::Service::on_connectionEstablished(const msockaddr_in& remote_addr)
{
    MUTEX_INSPECTOR;
    S_LOG("on_connectionEstablished");
    stopAlarm(refTimer::T_001_common_connect_failed,nullptr,ListenerBase::serviceId);

    DBG(logErr2("bool Service::on_connectionEstablished( ) !!!--- %s",remote_addr.dump().c_str()));

    logErr2(GREEN("dfsReferrer:connection established %s"),remote_addr.dump().c_str());

    rd.uplinkConnectionState->connectionEstablished=true;
    for(auto &i:rd.m_readyNotificationBackroutes)
    {
        passEvent(new dfsReferrerEvent::NotifyReferrerUplinkIsConnected(remote_addr,poppedFrontRoute(i)));
    }
    logErr2(GREEN("dfsReferrer:connection established %s"),remote_addr.dump().c_str());
    rd.uplinkConnectionState->uplink.insert(remote_addr);
//    logErr2(BLUE2("rd.uplinkConnectionState->uplink.insert %s"),remote_addr.dump().c_str());

    registerReferrer(remote_addr);

    for(auto &z:rd.uplinkConnectionState->ponged_for_cleanup_sockets)
    {
        if(z->remote_name()!=remote_addr)
        {
            DBG(logErr2("close %s",z->remote_name().dump().c_str()));
            z->close("cleanup unused connections");

        }
    }

    rd.uplinkConnectionState->ponged_for_cleanup_sockets.clear();

    d6_start(remote_addr);
    return true;
}
void dfsReferrer::Service::d4_on_disconnected(const msockaddr_in& sa)
{
    S_LOG("d4_on_disconnected "+sa.dump());
    MUTEX_INSPECTOR;


    DBG(logErr2("addr is uplink %d",rd.uplinkConnectionState->uplink.count(sa)));
    if(rd.uplinkConnectionState->uplink.count(sa))
    {
        for(auto& z: rd.m_readyNotificationBackroutes)
        {
            passEvent(new dfsReferrerEvent::NotifyReferrerUplinkIsDisconnected(sa,poppedFrontRoute(z)));
        }
#ifdef CLOSE_DOWNLINKS
        auto downlinks=rd.urirefs->getDownlinks();
        for(auto &z: downlinks)
        {
            REF_getter<linkInfoDownReferrer> ld=z.second;
            {
                ld->esi_->close("uplink disconnected");
                for(auto& r: rd.m_readyNotificationBackroutes)
                {
                    passEvent(new dfsReferrerEvent::NotifyReferrerDownlinkDisconnected(z.first,poppedFrontRoute(r)));
                }
            }

        }
        rd.urirefs->clearDownlinks();;
#endif

        STAGE_D2_PING_NEIGHBOURS_start();
    }
}
void dfsReferrer::Service::d4_on_connect_failed(const msockaddr_in& sa)
{
    S_LOG("d4_on_connect_failed");
    MUTEX_INSPECTOR;
    DBG(logErr2("d4_on_connect_failed %s",sa.dump().c_str()));
    d4_on_disconnected(sa);

}

void dfsReferrer::Service::d4_uplink_mode(const REF_getter<epoll_socket_info>& esi, const msockaddr_in &visibleName)
{
    S_LOG("d4_uplink_mode");
    MUTEX_INSPECTOR;
    DBG(logErr2("switch mode"));

    rd.stage=_dfsReferrerData::_stage::STAGE_D4_MAINTAIN_CONNECTION;

//    if(!rd.externalAccessIsPossible && m_enableExternalCheckConnection)
    if(false) {
//        d5_start_external_connection_check(esi,visibleName);
    }
    else
    {
    }

    on_connectionEstablished(esi->remote_name());


}

void dfsReferrer::Service::d6_start(const msockaddr_in& sa)
{
    rd.stage=_dfsReferrerData::_stage::STAGE_D6_MAINTAIN_CONNECTION;
    S_LOG("d6_start "+sa.dump());
    MUTEX_INSPECTOR;
    DBG(logErr2("d6_start send PT_CAPS_SHORT"));
    sendEvent(sa,ServiceEnum::DFSReferrer,
              new dfsReferrerEvent::Ping(dfsReferrer::PingType::PT_CAPS_SHORT,
                                         iInstance->globalCookie(),
                                         getRpcExternalListenPortMain(iInstance),
                                         getRpcInternalListenAddrs(iInstance),
                                         iUtils->getNow(),
                                         rd.connection_sequence_id,
                                         dfsReferrerEvent::Ping::CT_NODE,
                                         ListenerBase::serviceId));
    _reset_alarm(T_007_D6_resend_ping_caps_short,sa);
}
bool dfsReferrer::Service::d6_on_pong_PT_CAPS_SHORT(const REF_getter<epoll_socket_info>& esi)
{
    MUTEX_INSPECTOR;
    S_LOG("d6_on_pong_PT_CAPS_SHORT");
    {
        auto &ra=esi->remote_name();
        _stop_alarm(T_007_D6_resend_ping_caps_short,ra);
        _stop_alarm(T_008_D6_resend_ping_caps_long,ra);
        _stop_alarm(T_009_pong_timed_out_caps_long,ra);

        _reset_alarm(T_008_D6_resend_ping_caps_long,ra);
        _reset_alarm(T_009_pong_timed_out_caps_long,ra);

    }
    return true;

}
void dfsReferrer::Service::reset_T_001_common_connect_failed()
{
    MUTEX_INSPECTOR;
    resetAlarm(refTimer::T_001_common_connect_failed,nullptr,outV(rd.connection_sequence_id).get(),ListenerBase::serviceId);
}
bool dfsReferrer::Service::isTopServer(const msockaddr_in& sa)
{
    MUTEX_INSPECTOR;
    return static_cast<bool>(rd.uplinkConnectionState->saCapsFromConfig.count(sa));
}

void dfsReferrer::Service::STAGE_D21_PING_CAPS_start()
{
    MUTEX_INSPECTOR;
    rd.stage=_dfsReferrerData::_stage::STAGE_D21_PING_CAPS;
    for(auto &caps:rd.uplinkConnectionState->saCapsFromConfig)
    {
        auto external=getRpcExternalListenPortMain(iInstance);
        auto internal=getRpcInternalListenAddrs(iInstance);
        auto gcid=iInstance->globalCookie();
        sendEvent(caps,ServiceEnum::DFSReferrer,
                  new dfsReferrerEvent::Ping(dfsReferrer::PingType::PT_CACHED, gcid,external,internal,iUtils->getNow(),
                                             rd.connection_sequence_id,
                                             dfsReferrerEvent::Ping::CT_NODE,
                                             ListenerBase::serviceId));
    }
    reset_T_001_common_connect_failed();

}
void dfsReferrer::Service::STAGE_D2_PING_NEIGHBOURS_start()
{
    MUTEX_INSPECTOR;
    S_LOG("STAGE_D2_PING_NEIGHBOURS_start "+iInstance->getName());
    rd.connection_sequence_id++;

    rd.stage=_dfsReferrerData::_stage::STAGE_D2_PING_NEIGHBOURS;


    if(!rd.uplinkConnectionState.valid())
        throw CommonError("if(!uplinkConnectionState.valid())");
    rd.uplinkConnectionState=new _uplinkConnectionState(rd.uplinkConnectionState->m_isTopServer,rd.uplinkConnectionState->saCapsFromConfig);
    DBG(logErr2("d2_start"));

    if(rd.uplinkConnectionState->m_isTopServer)
    {
        DBG(logErr2("uplinkConnectionState->m_isTopServer NO d2 start"));
        return;
    }

    if(time(nullptr) - rd.d2_start_time<2)
    {
        DBG(logErr2("if(time(NULL) - d2_start_time<2)"));
        sleep(1);
    }
    rd.d2_start_time=time(nullptr);
    reset_T_001_common_connect_failed();



    std::vector<msockaddr_in> s=rd.neighbours.getAllAndClear();
    if(s.size())
    {

        for(auto& i:s)
        {
//            logErr2("ping %s",i.dump().c_str());
            if(isTopServer(i)) continue;
            sendEvent(i,ServiceEnum::DFSReferrer,
                      new dfsReferrerEvent::Ping(dfsReferrer::PingType::PT_CACHED, iInstance->globalCookie(),
                                                 getRpcExternalListenPortMain(iInstance),getRpcInternalListenAddrs(iInstance),
                                                 iUtils->getNow(),
                                                 rd.connection_sequence_id,
                                                 dfsReferrerEvent::Ping::CT_NODE,
                                                 ListenerBase::serviceId));
        }
        _reset_alarm(T_040_D2_cache_pong_timed_out_from_neighbours);
    }
    else
    {
        STAGE_D21_PING_CAPS_start();
    }

}

void dfsReferrer::Service::STAGE_D3_GET_GEFERRERS_start(const msockaddr_in& sa)
{
    MUTEX_INSPECTOR;
    rd.stage=_dfsReferrerData::_stage::STAGE_D3_GET_GEFERRERS;

    S_LOG("d3_start "+sa.dump());
    DBG(logErr2("d3_start"));

    {
        std::set<msockaddr_in> sas=iInstance->myExternalAddressesGet();
        if(sas.empty())
            throw CommonError("if(sas.size()==0)");

        DBG(logErr2("dfsCapsEvent::GetReferrersREQ "));

        REF_getter<Event::Base> ze=new dfsCapsEvent::GetReferrersREQ(sas,ListenerBase::serviceId);
        sendToplinkReq(sa,new dfsReferrerEvent::ToplinkDeliverREQ(ServiceEnum::DFSCaps,ze.get(),ListenerBase::serviceId));
        _reset_alarm(T_002_D3_caps_get_service_request_is_timed_out);
    }
    reset_T_001_common_connect_failed();

}
bool dfsReferrer::Service::on_UpdateConfigREQ(const dfsReferrerEvent::UpdateConfigREQ*e)
{
    MUTEX_INSPECTOR;
    std::map<SOCKET_id, REF_getter<linkInfoDownReferrer> >ls=rd.urirefs->getDownlinks();
    for(auto& x: ls)
    {
        route_t br;
        {
            br=x.second->backRoute_;
        }
        passEvent(new dfsReferrerEvent::UpdateConfigRSP(e->bod,poppedFrontRoute(br)));
    }
    rd.config_bod=e->bod;
    ConfigDB_private c;
    c.set("refferer_config",rd.config_bod);

    return true;
}
bool dfsReferrer::Service::on_UpdateConfigRSP(const dfsReferrerEvent::UpdateConfigRSP*e)
{
    MUTEX_INSPECTOR;
    rd.config_bod=e->bod;
    std::map<SOCKET_id, REF_getter<linkInfoDownReferrer> >ls=rd.urirefs->getDownlinks();
    for(auto& x: ls)
    {
        route_t br;
        {
            br=x.second->backRoute_;
        }
        passEvent(new dfsReferrerEvent::UpdateConfigRSP(e->bod,poppedFrontRoute(br)));
    }
    return true;

}

