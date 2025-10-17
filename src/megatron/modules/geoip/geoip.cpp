#include "IUtils.h"
void registerGEOIP(const char* pn);

IUtils *iUtils;
extern "C" void
#ifdef DEBUG
registerModuleDebug
#else
registerModule
#endif
(IUtils* f, const char*pn)
{

    iUtils=f;

    registerGEOIP(pn);

}

