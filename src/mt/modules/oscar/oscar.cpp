
#include "IUtils.h"
IUtils *iUtils;
void registerOscarModule(const char* );

extern "C" void
#ifdef DEBUG
registerModuleDebug
#else
registerModule
#endif
(IUtils* f, const char*pn)
{

    iUtils=f;
    registerOscarModule(pn);
}

