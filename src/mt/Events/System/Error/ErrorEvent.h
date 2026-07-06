#pragma once
#include "SERVICE_id.h"
#include "route_t.h"
#include "EVENT_id.h"
#include "event_mt.h"

#include "ghash.h"
namespace ErrorEventEnum
{
    const EVENT_id ErrorRSP(ghash("@g_ErrorRSP"));
}

namespace ErrorEvent
{


    class ErrorRSP: public Event::Base
    {


    public:
        static Base* construct(const route_t &r)
        {
            return new ErrorRSP(r);
        }
        ErrorRSP(int _errCode, const std::string& _errtext,
                 const route_t&r)
            :Base(ErrorEventEnum::ErrorRSP,r),errcode(_errCode),errText(_errtext) {}
        ErrorRSP(int _errCode, const std::string& _action
                )
            :Base(ErrorEventEnum::ErrorRSP),errcode(_errCode),errText(_action) {}
        ErrorRSP(const route_t&r)
            :Base(ErrorEventEnum::ErrorRSP,r) {}
        int errcode;
        std::string errText;
        void unpack(inBuffer& o)
        {
            o>>errcode;
            o>>errText;
        }
        void pack(outBuffer&o) const
        {
            o<<errcode<<errText;
        }

    };

}
