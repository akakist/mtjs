#include "IUtils.h"
void registerHTTPModule(const char*);

IUtils *iUtils;
void aaa();
extern "C" void
#ifdef DEBUG
registerModuleDebug
#else
registerModule
#endif
(IUtils* f, const char*pn)
{

    iUtils=f;
    registerHTTPModule(pn);
}

