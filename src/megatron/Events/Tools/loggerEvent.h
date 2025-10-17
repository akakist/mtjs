#pragma once
#include "EVENT_id.h"
#include "SERVICE_id.h"
#include "event_mt.h"
#include "route_t.h"


#include "genum.hpp"




namespace ServiceEnum
{
    const SERVICE_id Logger(genum_Logger);

}
namespace loggerEventEnum
{
    const EVENT_id LogMessage(genum_loggerLogMessage);
}


namespace loggerEvent
{

    class LogMessage: public Event::Base
    {



    public:
        static Base* construct(const route_t &r)
        {
            return new LogMessage(r);
        }
        LogMessage(const std::string &_opcode, const std::string& _msg)
            :Base(loggerEventEnum::LogMessage),opcode(_opcode),msg(_msg) {}
        LogMessage(const route_t&r)
            :Base(loggerEventEnum::LogMessage,r) {}
        std::string opcode;
        std::string msg;
        void unpack(inBuffer& o)
        {
            o>>opcode>>msg;
        }
        void pack(outBuffer&o) const
        {
            o<<opcode<<msg;
        }

    };



}
