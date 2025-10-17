#pragma once
#include "SERVICE_id.h"
#include "genum.hpp"
namespace Ifaces
{
/// this class is used to create simple interfaces, shared between services

    const SERVICE_id AV(genum_AV);
    const SERVICE_id SSL(genum_SSL);
    const SERVICE_id GEOIP(genum_GEOIP);
    class Base
    {
    public:
        virtual ~Base() {}
    };
};
