#include "IUtils.h"

IUtils *iUtils;
void registerWebSocketClientService(const char*);

extern "C" void
#ifdef DEBUG
registerModuleDebug
#else
registerModule
#endif
(IUtils* f, const char* pn)
{

    iUtils=f;
    registerWebSocketClientService(pn);
}
