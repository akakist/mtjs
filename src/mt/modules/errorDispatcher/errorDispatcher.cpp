#include "IUtils.h"

IUtils *iUtils;
void registerErrorDispatcherService(const char*);

extern "C" void
#ifdef DEBUG
registerModuleDebug
#else
registerModule
#endif
(IUtils* f, const char* pn)
{

    iUtils=f;
    registerErrorDispatcherService(pn);
}
