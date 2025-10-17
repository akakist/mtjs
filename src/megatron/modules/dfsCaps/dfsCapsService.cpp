
#include "dfsCapsService.h"
#include <stdarg.h>
#include "IGEOIP.h"
#include "logging.h"
#include "mutexInspector.h"
#include "tools_mt.h"

#include <math.h>
#include <cmath>

#include <sys/types.h>
#include <sys/stat.h>
#include "Events/System/Net/rpcEvent.h"
#include "Events/System/timerEvent.h"

#include "events_dfsCaps.hpp"

#define CAPS_DBNAME "caps"
dfsCaps::Service::Service(const SERVICE_id &svs, const std::string&  nm, IInstance *_ifa):
    UnknownBase(nm),
    ListenerBuffered1Thread(nm,svs),
    Broadcaster(_ifa),

    TimerHelper(_ifa),iInstance(_ifa)
{
    MUTEX_INSPECTOR;
}
dfsCaps::Service::~Service() {

}
UnknownBase* dfsCaps::Service::construct(const SERVICE_id& id, const std::string&  nm, IInstance *_ifa)
{
    return new Service(id,nm,_ifa);
}
void registerDFSCapsService(const char* pn)
{
    if(pn)
    {
        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_SERVICE,ServiceEnum::DFSCaps,"DFSCaps",getEvents_dfsCaps());
    }
    else
    {

        iUtils->registerService(ServiceEnum::DFSCaps,dfsCaps::Service::construct,"DFSCaps");
        regEvents_dfsCaps();
    }
}
bool dfsCaps::Service::on_GetCloudRootsREQ(const dfsCapsEvent::GetCloudRootsREQ* e, const dfsReferrerEvent::ToplinkDeliverREQ *e_toplink)
{
    MUTEX_INSPECTOR;
    if(e->externalListenAddr.size()==0)
    {
        logErr2("if(E->externalListenAddr.size()==0) %s",__func__);
        return false;
    }
    geoNetRec res;
    I_GEOIP *geoip=(I_GEOIP*)iUtils->queryIface(Ifaces::GEOIP);
    std::vector<msockaddr_in> sv;
    for(auto& z: e->externalListenAddr)
    {
        if(geoip->findNetRec(z.getStringAddr(),z.family()==AF_INET,res))
        {
            auto ret=cloud_roots.findReferrers(res.lat,res.lon,z.family());
            for(auto &x:ret)
            {
                sv.push_back(x->sa);
            }

        }
        else
        {
            auto ret=cloud_roots.findReferrers(LOCATION_LATITUDE_UNDEF,LOCATION_LONGITUDE_UNDEF,z.family());
            for(auto &x:ret)
            {
                sv.push_back(x->sa);
            }
        }

    }
    REF_getter<Event::Base> re=new dfsReferrerEvent::ToplinkDeliverRSP(new dfsCapsEvent::GetCloudRootsRSP(sv,e->route),poppedFrontRoute(e_toplink->route));
    passEvent(re);
    return true;

}

bool dfsCaps::Service::on_GetReferrersREQ(const dfsCapsEvent::GetReferrersREQ* E_getReferrersReq,const dfsReferrerEvent::ToplinkDeliverREQ* e_toplink)
{

    MUTEX_INSPECTOR;
    if(E_getReferrersReq->externalListenAddr.size()==0)
    {
        logErr2("if(E->externalListenAddr.size()==0) %s",__func__);
        return false;
    }
    geoNetRec res;
    I_GEOIP *geoip=(I_GEOIP*)iUtils->queryIface(Ifaces::GEOIP);
    std::vector<msockaddr_in> sv;
    for(auto& z: E_getReferrersReq->externalListenAddr)
    {
        if(geoip->findNetRec(z.getStringAddr(),z.family()==AF_INET,res))
        {
            auto ret=referrer_nodes.findReferrers(res.lat,res.lon,z.family());
            for(auto &x:ret)
            {
                sv.push_back(x->sa);
            }

        }
        else
        {
            auto ret=referrer_nodes.findReferrers(LOCATION_LATITUDE_UNDEF,LOCATION_LONGITUDE_UNDEF,z.family());
            for(auto &x:ret)
            {
                sv.push_back(x->sa);
            }
        }

    }
    std::set<msockaddr_in> alreadySK;
    std::vector<msockaddr_in> sv1;

    for(auto& z: sv)
    {
        if(!alreadySK.count(z))
        {
            sv1.push_back(z);
            alreadySK.insert(z);
        }

    }
    passEvent(new dfsReferrerEvent::ToplinkDeliverRSP(new dfsCapsEvent::GetReferrersRSP(sv1,E_getReferrersReq->route),poppedFrontRoute(e_toplink->route)));
    return true;

}
bool dfsCaps::Service::on_RegisterMyRefferrerNodeREQ(const dfsCapsEvent::RegisterMyRefferrerNodeREQ* e)
{
    MUTEX_INSPECTOR;
    const dfsCapsEvent::RegisterMyRefferrerNodeREQ * e_RegisterMyRefferrerNodeREQ=e;//(const dfsCapsEvent::RegisterMyRefferrerNodeREQ *)z.get();
    if(e_RegisterMyRefferrerNodeREQ->externalListenAddr.size())
    {
        for(auto z:e_RegisterMyRefferrerNodeREQ->externalListenAddr)
        {
            I_GEOIP *geoip=(I_GEOIP*)iUtils->queryIface(Ifaces::GEOIP);
            geoNetRec res;
            if(geoip->findNetRec(z.getStringAddr(),z.family()==AF_INET,res))
            {
                referrer_nodes.addReferrer(res.lat,res.lon,z,e_RegisterMyRefferrerNodeREQ->downlinkCount,e_RegisterMyRefferrerNodeREQ->uplink);
            }
            else
            {
                /// undefined net
                referrer_nodes.addReferrer(LOCATION_LATITUDE_UNDEF,LOCATION_LONGITUDE_UNDEF,z,e_RegisterMyRefferrerNodeREQ->downlinkCount,e_RegisterMyRefferrerNodeREQ->uplink);
            }
        }

    }


    return true;

}
bool dfsCaps::Service::on_RegisterCloudRoot(const dfsCapsEvent::RegisterCloudRoot* e)
{
    MUTEX_INSPECTOR;
    S_LOG("on_RegisterCloudRoot");
    if(e->externalListenAddr.size())
    {
        for(auto z:e->externalListenAddr)
        {
            logErr2("addr %s",z.dump().c_str());
            I_GEOIP *geoip=(I_GEOIP*)iUtils->queryIface(Ifaces::GEOIP);
            geoNetRec res;
            if(geoip->findNetRec(z.getStringAddr(),z.family()==AF_INET,res))
            {
                cloud_roots.addReferrer(res.lat,res.lon,z,0,e->uplink);
            }
            else
            {
                /// undefined net
                cloud_roots.addReferrer(LOCATION_LATITUDE_UNDEF,LOCATION_LONGITUDE_UNDEF,z,0,e->uplink);
            }
        }

    }


    return true;

}

bool dfsCaps::Service::on_ToplinkDeliverREQ(const dfsReferrerEvent::ToplinkDeliverREQ* e_toplink)
{
    MUTEX_INSPECTOR;
    S_LOG("on_ToplinkDeliverREQ");
    REF_getter<Event::Base> z=e_toplink->getEvent();
    if(!z.valid())
        return false;

    if(z->id==dfsCapsEventEnum::GetReferrersREQ)
        return on_GetReferrersREQ((const dfsCapsEvent::GetReferrersREQ *)z.get(),e_toplink);
    if(z->id==dfsCapsEventEnum::RegisterMyRefferrerNodeREQ)
        return on_RegisterMyRefferrerNodeREQ((const dfsCapsEvent::RegisterMyRefferrerNodeREQ *)z.get());
    if(z->id==dfsCapsEventEnum::RegisterCloudRoot)
        return on_RegisterCloudRoot((const dfsCapsEvent::RegisterCloudRoot *)z.get());
    if(z->id==dfsCapsEventEnum::GetCloudRootsREQ)
        return on_GetCloudRootsREQ((const dfsCapsEvent::GetCloudRootsREQ *)z.get(),e_toplink);

    return true;
}

bool dfsCaps::Service::handleEvent(const REF_getter<Event::Base>& e)
{
    XTRY;
    MUTEX_INSPECTOR;
    S_LOG("dfsCaps::handleEvent");

    auto & ID=e->id;
    if(timerEventEnum::TickAlarm==ID)
        return on_TickAlarm((const timerEvent::TickAlarm*)e.get());
#ifdef WEBDUMP
    if( webHandlerEventEnum::RequestIncoming==ID)
        return on_RequestIncoming((const webHandlerEvent::RequestIncoming*)e.get());
#endif
    if(systemEventEnum::startService==ID)
        return on_startService((const systemEvent::startService*)e.get());
    if(dfsReferrerEventEnum::ToplinkDeliverREQ==ID)
        return on_ToplinkDeliverREQ((const dfsReferrerEvent::ToplinkDeliverREQ*)e.get());
    if( rpcEventEnum::IncomingOnConnector==ID)
    {
        S_LOG("rpcEventEnum::IncomingOnConnector");
        const rpcEvent::IncomingOnConnector* E=(const rpcEvent::IncomingOnConnector*)e.get();
        auto &IDC=E->e->id;
        if(dfsReferrerEventEnum::ToplinkDeliverREQ==IDC)
            return on_ToplinkDeliverREQ((const dfsReferrerEvent::ToplinkDeliverREQ*)E->e.get());

        logErr2("unhandled IncomingOnConnector event %s %s %d",iUtils->genum_name(E->e->id),__func__,__LINE__);
    }
    if( rpcEventEnum::IncomingOnAcceptor==ID)
    {
        S_LOG("rpcEventEnum::IncomingOnAcceptor");
        const rpcEvent::IncomingOnAcceptor* E=(const rpcEvent::IncomingOnAcceptor*)e.get();
        auto & IDA=E->e->id;
        if(dfsReferrerEventEnum::ToplinkDeliverREQ==IDA)
            return on_ToplinkDeliverREQ((const dfsReferrerEvent::ToplinkDeliverREQ*)E->e.get());

        logErr2("unhandled IncomingOnAcceptor event %s %s %d",iUtils->genum_name(E->e->id),__func__,__LINE__);
    }


    logErr2("unhandled event %s",iUtils->genum_name(e->id));
    XPASS;
    return false;
}

bool  dfsCaps::Service::on_startService(const systemEvent::startService*)
{
    S_LOG("on_startService");
    MUTEX_INSPECTOR;
    XTRY;

#ifdef WEBDUMP
    {
        sendEvent(ServiceEnum::WebHandler, new webHandlerEvent::RegisterDirectory("dfs","DFS"));
        sendEvent(ServiceEnum::WebHandler, new webHandlerEvent::RegisterHandler("dfs/Caps","Capabilities",ListenerBase::serviceId));
    }
#endif

    XPASS;
    return true;
}

#ifdef WEBDUMP
bool dfsCaps::Service::on_RequestIncoming(const webHandlerEvent::RequestIncoming* e)
{
    MUTEX_INSPECTOR;
    HTTP::Response cc(e->req);
    std::string out="<h1>DFS Caps report</h1><p>";


    out+="<pre>\n\n</pre>";

    cc.make_response(out);
    return true;
}
#endif
real dfsCaps::Service::Distance(const std::pair<real,real> & c1,const std::pair<real,real> & c2)
{
    MUTEX_INSPECTOR;
    real	radius = 6372795;
    real lat1=c1.first*M_PI/180;
    real lon1=c1.second*M_PI/180;
    real lat2=c2.first*M_PI/180;
    real lon2=c2.second*M_PI/180;

    real acos_rng = sin(lat1) * sin(lat2) + cos(lat1) * cos(lat2) * cos(lon2 - lon1);
    if (acos_rng >= 1. || std::isnan(acos_rng))
        return 0.;
    if (acos_rng <= -1.)
        return M_PI;
    return radius*(acos(acos_rng));
}


bool dfsCaps::Service::on_TickAlarm(const timerEvent::TickAlarm*e)
{

    logErr2("invalid alarm %d",e->tid);

    return true;
}
