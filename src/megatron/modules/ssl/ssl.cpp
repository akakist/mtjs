#include "IUtils.h"
void registerSSL(const char* pn);

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

    registerSSL(pn);

}

