#pragma once

#include "event_mt.h"

#include "ghash.h"
namespace systemEventEnum
{

    const EVENT_id startService(ghash("@g_startService"));
}
namespace systemEvent
{
    /**
     * \brief
     */
    class startService:public Event::NoPacked
    {
    public:


        static Base* construct(const route_t &)
        {
            return NULL;
        }
        startService():NoPacked( systemEventEnum::startService) {}
    };

}
