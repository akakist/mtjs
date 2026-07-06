#pragma once
#include "SERVICE_id.h"

#include "ghash.h"
namespace Ifaces
{
/// this class is used to create simple interfaces, shared between services

    const SERVICE_id AV(ghash("@g_AV"));
    const SERVICE_id SSL(ghash("@g_SSL"));
    const SERVICE_id GEOIP(ghash("@g_GEOIP"));
    class Base
    {
    public:
        virtual ~Base() {}
    };
};
