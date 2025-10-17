#include <string>
#include <map>
#include "SERVICE_id.h"
#include "postgresService.h"
#include "IUtils.h"
IUtils *iUtils;
//#void registerPostgresModule();
void registerPostgresModule(const char*pn);

extern "C" void
#ifdef DEBUG
registerModuleDebug
#else
registerModule
#endif
(IUtils* f, const char* pn)
{

    iUtils=f;
    registerPostgresModule(pn);
}

