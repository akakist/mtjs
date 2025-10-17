#include "IUtils.h"
#ifdef WEBDUMP
#include <string>
#include <map>

IUtils *iUtils;
void registerWebHandlerModule(const char*);

extern "C" void
#ifdef DEBUG
registerModuleDebug
#else
registerModule
#endif
(IUtils* f, const char* pn)
{
    iUtils=f;
    registerWebHandlerModule(pn);

}

#endif
