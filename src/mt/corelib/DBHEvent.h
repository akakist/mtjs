#pragma once

#include "ghash.h"
namespace ServiceEnum
{
    const SERVICE_id Mysql(ghash("@g_Mysql"));
    const SERVICE_id Postgres(ghash("@g_Postgres"));

}
