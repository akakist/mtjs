#ifndef _________dfsReferrerEvent_h123Z1
#define _________dfsReferrerEvent_h123Z1
#include "EVENT_id.h"
#include "SERVICE_id.h"
namespace ServiceEnum
{
    const SERVICE_id prodtestServer(genum_prodtestServer);
    const SERVICE_id prodtestServerWeb(genum_prodtestServerWeb);
}

namespace prodtestEventEnum
{

    const EVENT_id AddTaskREQ(genum_pt_AddTaskREQ);
    const EVENT_id AddTaskRSP(genum_pt_AddTaskRSP);

}


#endif
