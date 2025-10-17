#include "corelib/mutexInspector.h"
#include <time.h>
#include <map>
#include "testTimerService.h"
#include "version_mega.h"
#include "Events/System/timerEvent.h"
#include "refstring.h"
#include "IUtils.h"
#include "___testEvent.h"
#include "commonError.h"


std::set<std::string> samples;
bool testTimer::Service::on_startService(const systemEvent::startService*)
{
    MUTEX_INSPECTOR;

    for(int i=0; i<10; i++)
    {
        sendEvent(ServiceEnum::Timer,new timerEvent::SetAlarm(1,toRef(std::to_string(i)),toRef("any additional data "+std::to_string(i)).get(),0.1*i,ListenerBase::serviceId));
        samples.insert(std::to_string(i));
    }

    return true;
}


bool testTimer::Service::handleEvent(const REF_getter<Event::Base>& e)
{
    XTRY;
    try {
        MUTEX_INSPECTOR;
        auto& ID=e->id;
        if(systemEventEnum::startService==ID)
            return on_startService((const systemEvent::startService*)e.get());
        if(timerEventEnum::TickAlarm==ID)
        {
            auto E=static_cast<timerEvent::TickAlarm*>(e.get());

            if(E->tid==1) ///
            {
                auto &data=E->data->container;
                refbuffer* b=(refbuffer*)E->cookie.get();
                auto &cookie=b->container;
                logErr2("-> %d %s %s",E->tid,data.c_str(),cookie.c_str());
                samples.erase(data);
                if(samples.empty())
                {
                    iUtils->setTerminate(0);
                }
            }

            /// say engine we handled event, try return false to see behaviour
            return true;
        }


    }
    catch(const CommonError& e)
    {
        logErr2("testTimer CommonError  %s",e.what());

    }
    catch(const std::exception &e)
    {
        logErr2("testTimer std::exception  %s",e.what());
    }
    XPASS;
    /// we not handled event, engine will log it
    return false;
}

testTimer::Service::~Service()
{
}


testTimer::Service::Service(const SERVICE_id& id, const std::string& nm,IInstance* ins):
    UnknownBase(nm),
    ListenerBuffered1Thread(nm,id),
    Broadcaster(ins), iInstance(ins)
{


}

void registerTestTimer(const char* pn)
{
    MUTEX_INSPECTOR;

    XTRY;
    if(pn)
    {
        std::set<EVENT_id> es;
        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_SERVICE,ServiceEnum::testTimer,"testTimer",es);
    }
    else
    {
        iUtils->registerService(ServiceEnum::testTimer,testTimer::Service::construct,"testTimer");
//        regEvents_prodtestTimer();
    }
    XPASS;
}







