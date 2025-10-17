#pragma once
#include "EVENT_id.h"
#include "Real.h"
#include "SERVICE_id.h"
#include "event_mt.h"
#include "genum.hpp"
namespace ServiceEnum
{
    const SERVICE_id Timer(genum_Timer);
}

namespace timerEventEnum
{
    const EVENT_id SetTimer(genum_timerSetTimer);
    const EVENT_id SetAlarm(genum_timerSetAlarm);
    const EVENT_id ResetAlarm(genum_timerResetAlarm);
    const EVENT_id StopTimer(genum_timerStopTimer);
    const EVENT_id StopAlarm(genum_timerStopAlarm);
    const EVENT_id TickTimer(genum_timerTickTimer);
    const EVENT_id TickAlarm(genum_timerTickAlarm);
}

namespace timerEvent
{

    class TickTimer: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        const int tid;
        REF_getter<refbuffer> data;
        REF_getter<Refcountable> cookie;
        TickTimer(const int& _tid,const REF_getter<refbuffer>& _data,const REF_getter<Refcountable>& _cookie, const route_t & r):NoPacked(timerEventEnum::TickTimer,r),
            tid(_tid),data(_data),cookie(_cookie)
        {
            if(!data.valid())
                data=new refbuffer;
            if(!cookie.valid())
                cookie=new refbuffer;

        }
    };

    class TickAlarm: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        const int tid;
        REF_getter<refbuffer> data;
        REF_getter<Refcountable> cookie;
        TickAlarm(const int& _tid,const REF_getter<refbuffer>& _data,const REF_getter<Refcountable>& _cookie, const route_t & r):NoPacked(timerEventEnum::TickAlarm,r),
            tid(_tid),data(_data),cookie(_cookie)
        {
            if(!data.valid())
                data=new refbuffer;
            if(!cookie.valid())
                cookie=new refbuffer;

        }
    };

    class StopTimer:public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        const int tid;
        REF_getter<refbuffer>data;
        StopTimer(const int _id, const REF_getter<refbuffer>& _data,const route_t &r)
            :NoPacked(timerEventEnum::StopTimer,r), tid(_id), data(_data)
        {
            if(!data.valid())
                data=new refbuffer;
        }
    };

    class StopAlarm:public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        int tid;
        REF_getter<refbuffer> data;
        StopAlarm(const int& _id, const REF_getter<refbuffer>& _data,const route_t &r)
            :NoPacked(timerEventEnum::StopAlarm,r), tid(_id), data(_data)
        {
            if(!data.valid())
                data=new refbuffer;
        }
    };

    class SetTimer:public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        const int tid;
        REF_getter<refbuffer> data;
        REF_getter<Refcountable> cookie;
        const real delay_secs;
        SetTimer(const int &_id, const REF_getter<refbuffer>& _data, const REF_getter<Refcountable>& _cookie, const real&  _delay_secs, const route_t& r)
            :NoPacked(timerEventEnum::SetTimer,r),
             tid(_id),data(_data),cookie(_cookie),
             delay_secs(_delay_secs)
        {
            if(!data.valid())
                data=new refbuffer;
            if(!cookie.valid())
                cookie=new refbuffer;

        }
    };
    class SetAlarm:public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        const int tid;
        REF_getter<refbuffer> data;
        REF_getter<Refcountable> cookie;
        const real delay_secs;
        SetAlarm(const int &_id, const REF_getter<refbuffer>& _data, const REF_getter<Refcountable>& _cookie, const real&  _delay_secs, const route_t& r)
            :NoPacked(timerEventEnum::SetAlarm,r),
             tid(_id),data(_data),cookie(_cookie),
             delay_secs(_delay_secs)
        {
            if(!data.valid())
                data=new refbuffer;
            if(!cookie.valid())
                cookie=new refbuffer;

        }
    };

    class ResetAlarm:public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &)
        {
            return NULL;
        }
        const int tid;
        REF_getter<refbuffer> data;
        REF_getter<Refcountable> cookie;
        const real delay_secs;
        ResetAlarm(const int &_id, const REF_getter<refbuffer>& _data, const REF_getter<Refcountable>& _cookie, const real&  _delay_secs, const route_t& r)
            :NoPacked(timerEventEnum::ResetAlarm,r),
             tid(_id),data(_data),cookie(_cookie),
             delay_secs(_delay_secs)
        {
            if(!data.valid())
                data=new refbuffer;
            if(!cookie.valid())
                cookie=new refbuffer;

        }
    };
}
