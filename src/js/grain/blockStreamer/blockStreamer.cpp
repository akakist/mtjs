
#include "IUtils.h"
IUtils *iUtils;
void registerBlockStreamerService(const char* pn);
extern "C" void
#ifdef _WIN32
__declspec(dllexport)
#endif
#ifdef DEBUG
registerModuleDebug
#else
registerModule
#endif
(IUtils* f,const char* pn)
{

    iUtils=f;
    registerBlockStreamerService(pn);
}

