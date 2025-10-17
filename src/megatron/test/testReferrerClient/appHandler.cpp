#include "appHandler.h"
#include <math.h>
#include <megatron_config.h>
#include "colorOutput.h"
#include "Events/DFS/referrerEvent.h"

void AppHandler::init(int argc, char ** argv, const std::string & filesDir, IInstance* instance)
{


    {

        std::set<msockaddr_in> caps;
        caps=instance->getConfig()->get_tcpaddr("CapsIP","127.0.0.1:10100","Root cloud address");

        for(auto& z:caps)
        {
            logErr2("caps %s",z.dump().c_str());
        }
        instance->sendEvent(ServiceEnum::ReferrerClient,new dfsReferrerEvent::InitClient(caps));
    }

}
AppHandler::AppHandler(IInstance* ins)
    :
    TimerHelper(ins),
    ObjectHandlerThreaded("BlockchainHandler",ins),
    isConnected(false),instance(ins)
{
    MUTEX_INSPECTOR;
    try {
        MUTEX_INSPECTOR;

        iUtils->registerEvent(
            (event_static_constructor) dfsReferrerEvent::UpdateConfigRSP::construct);
        iUtils->registerEvent(
            (event_static_constructor) dfsReferrerEvent::UpdateConfigREQ::construct);


        iUtils->registerEvent(
            (event_static_constructor) ErrorEvent::ErrorRSP::construct);
        iUtils->registerEvent(
            (event_static_constructor) dfsReferrerEvent::ToplinkDeliverRSP::construct);

        sendEvent(ServiceEnum::ReferrerClient,new dfsReferrerEvent::SubscribeNotifications(this));

    } catch(const std::exception &e)
    {
        logErr2("BlockchainHandler::BlockchainHandler: %s",e.what());
    }

}




bool AppHandler::on_NotifyReferrerUplinkIsConnected(const dfsReferrerEvent::NotifyReferrerUplinkIsConnected *e)
{

    XTRY;
    isConnected=true;
    referrer.insert(e->sa);

    /// signal to app terminate sucessfuly, test passed
    iUtils->setTerminate(0);
    return true;
    XPASS;
}


bool AppHandler::on_NotifyReferrerUplinkIsDisconnected(const dfsReferrerEvent::NotifyReferrerUplinkIsDisconnected *e)
{
    MUTEX_INSPECTOR;
    isConnected=false;
    referrer.erase(e->sa);
    return true;

}
bool AppHandler::on_ToplinkDeliverRSP(const dfsReferrerEvent::ToplinkDeliverRSP* e_tl)
{
    MUTEX_INSPECTOR;
    XTRY;
    REF_getter<Event::Base> z=e_tl->getEvent();
    auto &ID=z->id;
    if(ErrorEventEnum::ErrorRSP==ID)
    {
        ErrorEvent::ErrorRSP*er=(ErrorEvent::ErrorRSP*)z.get();
        logErr2("Error %s",er->errText.c_str());
        std::string javaCookie=e_tl->route.getLastJavaCookie();

        return true;
    }

    XPASS;
    return false;
}

bool AppHandler::OH_handleObjectEvent(const REF_getter<Event::Base>& e)
{
    MUTEX_INSPECTOR;
    try {

//        logErr2("OH_handleObjectEvent %s",e->dump().toStyledString().c_str());
        auto& ID=e->id;
        if(dfsReferrerEventEnum::UpdateConfigRSP==ID)
        {
            MUTEX_INSPECTOR;
            const dfsReferrerEvent::UpdateConfigRSP*xe=(const dfsReferrerEvent::UpdateConfigRSP*)e.get();
            config_bod=xe->bod;
            return true;
        }
        if( dfsReferrerEventEnum::ToplinkDeliverRSP==ID)
            return on_ToplinkDeliverRSP((const dfsReferrerEvent::ToplinkDeliverRSP*)e.get());
        if( dfsReferrerEventEnum::NotifyReferrerUplinkIsConnected==ID)
            return on_NotifyReferrerUplinkIsConnected((const dfsReferrerEvent::NotifyReferrerUplinkIsConnected*)e.get());
        if( dfsReferrerEventEnum::NotifyReferrerUplinkIsDisconnected==ID)
            return on_NotifyReferrerUplinkIsDisconnected((const dfsReferrerEvent::NotifyReferrerUplinkIsDisconnected*)e.get());

        if(timerEventEnum::TickAlarm==ID)
            return on_TickAlarm((const timerEvent::TickAlarm*)e.get());

        if(rpcEventEnum::IncomingOnConnector==ID)
        {
            MUTEX_INSPECTOR;
            const rpcEvent::IncomingOnConnector*ze=(const rpcEvent::IncomingOnConnector*)e.get();
            auto& IDC=ze->e->id;
            if(dfsReferrerEventEnum::UpdateConfigRSP==IDC)
            {
                MUTEX_INSPECTOR;
                const dfsReferrerEvent::UpdateConfigRSP*xe=(const dfsReferrerEvent::UpdateConfigRSP*)ze->e.get();
                config_bod=xe->bod;
                return true;
            }


        }

    }
    catch(const std::exception &e)
    {
        logErr2("exception %s",e.what());
    }
    catch(...)
    {
        logErr2("exception ...");
    }
    return false;
}
bool AppHandler::on_TickAlarm(const timerEvent::TickAlarm*)
{
    MUTEX_INSPECTOR;
    return false;
}

