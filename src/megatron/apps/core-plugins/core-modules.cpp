#include "IUtils.h"
#include <string>
#include <map>
void registerHTTPModule(const char*);
void registerHTTPModule(const char*);
void registerObjectProxyModule(const char*);
void registerOscarModule(const char*);
// void registerOscarSecureModule(const char*);
void registerRPCService(const char*);
void registerSocketModule(const char*);
void registerSqliteModule(const char*);
void registerTelnetService(const char*);
void registerTimerService(const char*);
void registerWebHandlerModule(const char*);
void registerErrorDispatcherService(const char*);
void registerSSL(const char*);

IUtils *iUtils;
void aaa();
extern "C" void
#ifdef DEBUG
registerModuleDebug
#else
registerModule
#endif
(IUtils* f,const char* pn)
{

    iUtils=f;
    registerHTTPModule(pn);
//    registerLoggerService(pn);
    registerObjectProxyModule(pn);
    registerOscarModule(pn);
    // registerOscarSecureModule(pn);
    registerRPCService(pn);
    registerSocketModule(pn);
    registerTelnetService(pn);
    registerTimerService(pn);
    registerWebHandlerModule(pn);
    registerErrorDispatcherService(pn);
    registerSSL(pn);

}

