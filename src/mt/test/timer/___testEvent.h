#pragma once

#include "SERVICE_id.h"

#include "ghash.h"
namespace ServiceEnum
{
    /// genum_ prefix needed to scan all occurence in files */*Event.h
    /// end make genum.hpp to make unique enumeration of services and events.
    const SERVICE_id testTimer(ghash("@g_testTimer"));
}



