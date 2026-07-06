#ifndef _________dfsReferrerEvent_h123Z1
#define _________dfsReferrerEvent_h123Z1
#include "EVENT_id.h"
#include "SERVICE_id.h"
#include "ghash.h"
namespace ServiceEnum
{
    const SERVICE_id prodtestServer(ghash("@g_prodtestServer"));
    const SERVICE_id prodtestServerWeb(ghash("@g_prodtestServerWeb"));
}

namespace prodtestEventEnum
{

    const EVENT_id AddTaskREQ(ghash("@g_pt_AddTaskREQ"));
    const EVENT_id AddTaskRSP(ghash("@g_pt_AddTaskRSP"));

}


#endif
