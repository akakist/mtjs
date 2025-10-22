#include "timerService.h"
#include "events_timer.hpp"
#include "colorOutput.h"
#include "mutexInspector.h"
#include "commonError.h"
#if !defined __MOBILE__ && !defined __FreeBSD__
#include <sys/timeb.h>
#endif
#if !defined __ANDROID_API__ && !defined __FreeBSD__
#include <sys/timeb.h>
#endif
//int Timer::task::total=0;
int64_t getNow()
{


    uint64_t now;
#if defined(_WIN32)
    timeb tb;
    ftime(&tb);
    uint64_t add=tb.millitm;
    add*=1000;
    now=tb.time;
    now*=1000000;
    now+=add;
    Integer v;
    v.set(now);
    return v;
#else
    timespec tstmp;
    if(clock_gettime(CLOCK_REALTIME,&tstmp))
    {
        throw CommonError("clock_gettime: errno %d",errno);
    }
    auto _1=tstmp.tv_sec;
    _1*=1000000;
    auto _2=tstmp.tv_nsec;
    _2/=1000;

    now= static_cast<uint64_t>(_1);
    now+=_2;
    return now;
#endif


}



Timer::Service::Service(const SERVICE_id& id, const std::string& nm, IInstance* ifa):
    UnknownBase(nm),

    ListenerBuffered1Thread(nm,id),
    Broadcaster(ifa),
    all(new _all),
    nexts(new _nexts),
    iInstance(ifa),
    m_isTerminating(false)

{
}
void Timer::Service::deinit()
{
    m_isTerminating=true;
    nexts->clear();
    all->clear();
    {
        nexts->m_condition.broadcast();
    }
    int err=pthread_join(m_pt_of_thread,NULL);
    if(err)
    {
        printf(RED("%s pthread_join: %s"),__PRETTY_FUNCTION__,strerror(errno));
    }

    ListenerBuffered1Thread::deinit();
}

Timer::Service::~Service()
{


}
bool Timer::Service::on_startService(const systemEvent::startService*)
{

    XTRY;
    if(pthread_create(&m_pt_of_thread,NULL,__worker,this))
    {
        throw CommonError("pthread_create: errno %d",errno);
    }
    XPASS;

    return true;
}

void* Timer::Service::__worker(void*p)
{

#ifdef __MACH__
    pthread_setname_np("Timer");
#else
#if !defined _WIN32 && !defined __FreeBSD__
    pthread_setname_np(pthread_self(),"Timer");
#endif
#endif

    static_cast<Timer::Service*>(p)->worker();
    return NULL;
}

void Timer::Service::worker()
{
    m_pt_of_thread=pthread_self();
    while (1)
    {
        try
        {
            {
                {
                    XTRY;
                    if(m_isTerminating) return;

                    {
                        auto n=nexts;
                        if(!n.valid())
                            return;
                        {
                            M_LOCKC(n->m_mutex);
                            if(n->mx_nexts.size()==0)
                            {
                                XTRY;
                                if(m_isTerminating) return;
                                //if(iInstance->isTerminating()) return;
                                n->m_condition.wait();
                                if(m_isTerminating) return;
                                //if(iInstance->isTerminating()) return;
                                XPASS;
                            }
                        }
                    }

                    while(1)
                    {
                        if(m_isTerminating) return;
                        int64_t itFirst;
                        std::deque<REF_getter<task> > itSecondCopy;
                        auto n=nexts;
                        auto a=all;
                        if(!n.valid())
                            return;
                        if(!a.valid())
                            return;
                        {
                            M_LOCKC(n->m_mutex);
                            if(m_isTerminating) return;
                            if(n->mx_nexts.size())
                            {
                                int64_t now=getNow();
                                auto it2=n->mx_nexts.begin();
                                itFirst=it2->first;
                                std::deque<REF_getter<task> > &itSecondRef=it2->second;
                                if(getNow()<itFirst)
                                {
                                    now=getNow();

                                    mtimespec ts;
                                    {
                                        int64_t tmp=itFirst;
                                        tmp/=1000000;
                                        {
                                            ts.tv_sec=(time_t)tmp;
                                            tmp=itFirst;
                                            tmp%=1000000;
                                            tmp*=1000;
                                        }
                                        ts.tv_nsec=(long)tmp;
                                    }

                                    XTRY;
                                    if(m_isTerminating) return;
                                    //if(iInstance->isTerminating()) return;
                                    {
                                        n->m_condition.timedwait(ts);
                                    }
                                    if(m_isTerminating) return;

                                    XPASS;
                                    continue;
                                }
                                else
                                {
                                    itSecondCopy=itSecondRef;
                                    n->mx_nexts.erase(itFirst);
                                }
                            }
                            if(m_isTerminating) return;

                        }
                        for(size_t i=0; i<itSecondCopy.size(); i++)
                        {
                            if(m_isTerminating) return;
                            REF_getter<task> t=itSecondCopy[i];
                            if(!t->erased)
                            {
                                if (t->type==task::TYPE_TIMER)
                                {
                                    XTRY;
                                    REF_getter<Event::Base> e=new timerEvent::TickTimer(t->id, t->data,t->cookie,t->destination);
                                    passEvent(e);
                                    {
                                        M_LOCKC(n->m_mutex);
                                        n->mx_nexts[getNow() + t->period_real*1000000].push_back(t);
                                    }
                                    XPASS;
                                }
                                else if (t->type==task::TYPE_ALARM)
                                {
                                    REF_getter<Event::Base> e=new timerEvent::TickAlarm(t->id,t->data, t->cookie, t->destination);
                                    passEvent(e);
                                    REF_getter<task> t=itSecondCopy[i];
                                    a->remove_t_only(t);
                                }
                            }
                            else
                            {
                                a->remove_t_only(t);
                            }
                        }
                    }
                    XPASS;
                }
            }
        }
        catch (std::exception& e)
        {
            logErr2("catched here %s",e.what());
        }
        catch (...)
        {
            logErr2("catched here");
        }
    }

}
bool Timer::Service::on_SetTimer(const timerEvent::SetTimer* ev)
{
    if(m_isTerminating)return false;
    auto n=nexts;
    auto a=all;
    if(!n.valid())
        return true;
    if(!a.valid())
        return true;
    route_t r=ev->route;
    r.pop_back();
    REF_getter<task> t=new task(task::TYPE_TIMER,ev->tid,ev->data,ev->cookie,r,ev->delay_secs);
    a->add(t);

    auto tb=getNow();
    real d=ev->delay_secs;
    d*=1000000;
    tb+=d;

    {
        M_LOCKC(n->m_mutex);
        n->mx_nexts[tb].push_back(t);
    }
    n->m_condition.signal();
    return true;
}
bool Timer::Service::on_SetAlarm(const timerEvent::SetAlarm* ev)
{
    XTRY;
    if(m_isTerminating)return false;
    auto n=nexts;
    auto a=all;
    if(!n.valid())
        return true;
    if(!a.valid())
        return true;

    route_t r=ev->route;
    r.pop_back();
    REF_getter<task> t=new task(task::TYPE_ALARM,ev->tid,ev->data,ev->cookie,r,ev->delay_secs);
    a->add(t);
    auto tb=getNow();
    real d=ev->delay_secs;
    d*=1000000;
    tb+=d;
    {
        XTRY;
        M_LOCKC(n->m_mutex);
        n->mx_nexts[tb].push_back(t);
        XPASS;
    }
    n->m_condition.signal();
    XPASS;
    return true;
}
bool Timer::Service::on_StopTimer(const timerEvent::StopTimer* ev)
{
    XTRY;
    if(m_isTerminating)return false;
    auto a=all;
    if(!a.valid())
        return true;
    route_t r=ev->route;
    r.pop_back();
    REF_getter<task> t=new task(task::TYPE_TIMER,ev->tid,ev->data,new refbuffer,r,0);
    a->remove(t);
    XPASS;
    return true;
}
bool Timer::Service::on_ResetAlarm(const timerEvent::ResetAlarm* ev)
{
    XTRY;
    if(m_isTerminating)return false;
    auto n=nexts;
    auto a=all;
    if(!n.valid())
        return true;
    if(!a.valid())
        return true;
    route_t r=ev->route;
    r.pop_back();
    REF_getter<task> t=new task(task::TYPE_ALARM,ev->tid,ev->data,ev->cookie,r,ev->delay_secs);
    a->replace(t);
    auto tb=getNow();
    real d=ev->delay_secs;
    d*=1000000;
    tb+=d;
    {
        M_LOCKC(n->m_mutex);
        n->mx_nexts[tb].push_back(t);
    }
    n->m_condition.signal();
    XPASS;
    return true;
}

bool Timer::Service::on_StopAlarm(const timerEvent::StopAlarm* ev)
{
    XTRY;
    if(m_isTerminating)return false;
    auto a=all;
    if(!a.valid())
        return true;
    route_t r=ev->route;
    r.pop_back();
    REF_getter<task> t=new task(task::TYPE_ALARM,ev->tid,ev->data,new refbuffer,r,0);
    a->remove(t);
    XPASS;
    return true;
}

void registerTimerService(const char* pn)
{
    if(pn)
    {
        iUtils->registerPlugingInfo(pn,IUtils::PLUGIN_TYPE_SERVICE,ServiceEnum::Timer,"Timer",getEvents_timer());
    }
    else
    {
        iUtils->registerService(ServiceEnum::Timer,Timer::Service::construct,"Timer");
        regEvents_timer();
    }
}

bool Timer::Service::handleEvent(const REF_getter<Event::Base>& e)
{
    MUTEX_INSPECTOR;
    XTRY;
    auto &ID=e->id;
    if(timerEventEnum::SetTimer==ID)
        return on_SetTimer((const timerEvent::SetTimer*)e.get());
    if( timerEventEnum::SetAlarm==ID)
        return on_SetAlarm((const timerEvent::SetAlarm*)e.get());
    if( timerEventEnum::ResetAlarm==ID)
        return on_ResetAlarm((const timerEvent::ResetAlarm*)e.get());
    if( timerEventEnum::StopTimer==ID)
        return on_StopTimer((const timerEvent::StopTimer*)e.get());
    if( timerEventEnum::StopAlarm==ID)
        return on_StopAlarm((const timerEvent::StopAlarm*)e.get());
    if(systemEventEnum::startService==ID)
        return on_startService((const systemEvent::startService*)e.get());

    XPASS;
    return false;

}



int Timer::_searchKey::operator < (const _searchKey& b) const
{
    const _searchKey& a=*this;
    if(a.t->id != b.t->id)
        return a.t->id < b.t->id;

    if(a.t->type != b.t->type)
        return a.t->type < b.t->type;

    if(!(a.t->destination == b.t->destination))
        return a.t->destination < b.t->destination;

    return a.t->data->container < b.t->data->container;
    return 0;
}
void Timer::_all::clear()
{
    M_LOCK(this);
    timers.clear();
}
int Timer::_all::exists(const REF_getter<task>& tt)
{
    M_LOCK(this);
    return timers.count(tt);
}
void Timer::_all::replace(const REF_getter<task>& t)
{
    M_LOCK(this);
    std::set<REF_getter<task> > &s=timers[t];
    for(auto& i:s)
    {
        REF_getter<task> task=i;
        task->erased=true;
    }
    timers.erase(t);
    timers[t].insert(t);
}
void Timer::_all::add(const REF_getter<task>& t)
{
    M_LOCK(this);
    timers[t].insert(t);
}
void Timer::_all::remove_t_only(const REF_getter<task>&t)
{
    M_LOCK(this);
    std::set<REF_getter<task> > &s=timers[t];
    if(s.count(t))
    {
        s.erase(t);
    }
}
void Timer::_all::remove(const REF_getter<task>& t)
{
    M_LOCK(this);
    std::set<REF_getter<task> > &s=timers[t];
    bool found=false;
    for(auto& i:s)
    {
        REF_getter<task> t=i;
        t->erased=true;
        found=true;
    }
    timers.erase(t);
    if(!found)
    {
    }
    else
    {
    }
}
