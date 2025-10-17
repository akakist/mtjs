#include "mtjsService.h"
bool MTJS::Service::AsyncExecuted(const mtjsEvent::AsyncExecuted* e)
{
    MUTEX_INSPECTOR;
    e->task->finalize(js_ctx);
    return true;
}
