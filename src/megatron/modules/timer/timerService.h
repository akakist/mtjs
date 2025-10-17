#pragma once
#include "mutexable.h"
#include "unknown.h"
#include "SERVICE_id.h"
#include <sys/time.h>
#include "IInstance.h"
#include "listenerBuffered1Thread.h"
#include "broadcaster.h"

#include <ostream>
#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/timerEvent.h"
#include "commonError.h"


namespace Timer
{

    struct task:public Refcountable
    {
        enum TYP
        {
            TYPE_ALARM,TYPE_TIMER,
        };
        const TYP type;
    private:
    public:
        const int id;
        const REF_getter<refbuffer> data;
        const REF_getter<Refcountable> cookie;
        const route_t destination;
        double period_real;
        bool erased;
        task(TYP t,const int& _id, const REF_getter<refbuffer> & _data, const REF_getter<Refcountable> & _cookie, const route_t& dst,const double& timeout)
            :type(t),id(_id),data(_data),cookie(_cookie),destination(dst),period_real(timeout),erased(false)
        {
        }
        ~task()
        {
        }
    };
    struct _searchKey
    {
        REF_getter<task> t;
        _searchKey(const REF_getter<task>& tt):t(tt) {}
        int operator < (const _searchKey& b) const;
    };
    struct _all: public Refcountable,public Mutexable
    {
        _all() {}
        std::map<_searchKey,std::set<REF_getter<task> > >   timers;
        void clear();
        int exists(const REF_getter<task>& tt);
        void replace(const REF_getter<task>& t);
        void add(const REF_getter<task>& t);
        void remove_t_only(const REF_getter<task>&t);
        void remove(const REF_getter<task>& t);
    };
    struct _nexts: public Refcountable
    {
        MutexC m_mutex;
        Condition m_condition;
        std::map<int64_t,std::deque<REF_getter<task> > > mx_nexts;
        _nexts()
            :m_condition(m_mutex)
        {
        }
        ~_nexts()
        {
        }
        void clear()
        {
            M_LOCKC(m_mutex);
            mx_nexts.clear();
        }
    };

    class Service:
        public UnknownBase,
        public ListenerBuffered1Thread,
        public Broadcaster
    {

        REF_getter<_all> all;

        REF_getter<_nexts> nexts;

        pthread_t m_pt_of_thread;
        IInstance *iInstance;
        bool handleEvent(const REF_getter<Event::Base>& e);

        Service(const SERVICE_id&, const std::string& nm, IInstance *ifa);
        ~Service();

        bool on_SetTimer(const timerEvent::SetTimer*);
        bool on_SetAlarm(const timerEvent::SetAlarm*);
        bool on_ResetAlarm(const timerEvent::ResetAlarm*);
        bool on_StopTimer(const timerEvent::StopTimer*);
        bool on_StopAlarm(const timerEvent::StopAlarm*);
        bool on_startService(const systemEvent::startService*);
    public:
        void deinit();

        static UnknownBase* construct(const SERVICE_id& id, const std::string&  nm,IInstance* ifa)
        {
            XTRY;
            return new Service(id,nm,ifa);
            XPASS;
        }
    private:
        void worker();
        static void *__worker(void*);

        bool m_isTerminating;
    };
};
