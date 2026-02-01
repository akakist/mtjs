#pragma once
#include "EVENT_id.h"
#include "SERVICE_id.h"
#include "event_mt.h"

#include "msockaddr_in.h"
#include <stdexcept>
#include <atomic>
#include <deque>
#include <unordered_map>
#include <set>
#include "Real.h"
#include "common/async_task.h"
#include "ioBuffer.h"
#include "eventEmitter.h"


namespace ServiceEnum
{
    const SERVICE_id mtjs(ghash("@g_mtjs"));

}


namespace mtjsEventEnum
{

    const EVENT_id AsyncExecuted(ghash("@g_mtjs_async_executed"));
    const EVENT_id Eval(ghash("@g_mtjs_Eval"));
    const EVENT_id EmitterData(ghash("@g_EmitterData"));
    const EVENT_id mtjsRpcREQ(ghash("@g_mtjsRpcREQ"));
    const EVENT_id mtjsRpcRSP(ghash("@g_mtjsRpcRSP"));
}


namespace mtjsEvent
{
    class AsyncExecuted: public Event::NoPacked
    {
    public:
        static Base* construct(const route_t &r)
        {
            return nullptr;
        }
        AsyncExecuted(const REF_getter<async_task> _t)
            :NoPacked(mtjsEventEnum::AsyncExecuted),
             task(_t)
        {}
        REF_getter<async_task> task;

    };







    class Eval: public Event::Base
    {

    public:
        static Base* construct(const route_t &r)
        {
            return new Eval(r);
        }
        Eval(const std::string& _js, const route_t & r)
            :Base(mtjsEventEnum::Eval,r),js(_js) {}
        Eval(const route_t&r)
            :Base(mtjsEventEnum::Eval,r) {}
        std::string js;
        void unpack(inBuffer& o)
        {
            o>>js;
        }
        void pack(outBuffer&o) const
        {
            o<<js;
        }

    };




    class EmitterData: public Event::NoPacked
    {

    public:
        static Base* construct(const route_t &r)
        {
            return nullptr;
        }
        EmitterData(const std::string&  _cmd, const std::string_view& _data, const REF_getter<EventEmitter> & _emitter)
            :NoPacked(mtjsEventEnum::EmitterData),cmd(_cmd),data(_data),emitter(_emitter) {}

        std::string cmd;
        std::string data;
        REF_getter<EventEmitter> emitter;

    };

    class mtjsRpcREQ: public Event::Base
    {

    public:
        static Base* construct(const route_t &r)
        {
            return new mtjsRpcREQ(r);
        }
        mtjsRpcREQ(const std::string& _method,const std::string& _params, const route_t & r)
            :Base(mtjsEventEnum::mtjsRpcREQ,r),method(_method),params(_params) {}
        mtjsRpcREQ(const route_t&r)
            :Base(mtjsEventEnum::mtjsRpcREQ,r) {}
        std::string params;
        std::string method;
        void unpack(inBuffer& o)
        {
            o>>method>>params;
        }
        void pack(outBuffer&o) const
        {
            o<<method<<params;
        }

    };
    class mtjsRpcRSP: public Event::Base
    {

    public:
        static Base* construct(const route_t &r)
        {
            return new mtjsRpcRSP(r);
        }
        mtjsRpcRSP(const std::string& _method,const std::string& _params, const route_t & r)
            :Base(mtjsEventEnum::mtjsRpcRSP,r),method(_method),params(_params) {}
        mtjsRpcRSP(const route_t&r)
            :Base(mtjsEventEnum::mtjsRpcRSP,r) {}
        std::string params;
        std::string method;
        void unpack(inBuffer& o)
        {
            o>>method>>params;
        }
        void pack(outBuffer&o) const
        {
            o<<method<<params;
        }

    };


}

