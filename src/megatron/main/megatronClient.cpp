#include "IInstance.h"
#define _FILE_OFFSET_BITS 64
#ifndef _MSC_VER
#include <unistd.h>
#include <dirent.h>
#endif
#include <sys/stat.h>
#include <stdio.h>
#ifdef _WIN32
#else
#include <dlfcn.h>
#endif
#include "megatronClient.h"
#include "CUtils.h"
#include "megatron_config.h"
#include "configObj.h"
#include "commonError.h"

//IUtils *iUtils=NULL;

#if !defined __MOBILE__

void registerHTTPModule(const char*);
void registerSqliteModule(const char*);
void registerTelnetService(const char*);

#ifdef WEBDUMP
void registerWebHandlerModule(const char*);
#endif
void registerErrorDispatcherService(const char*);
#endif

void registerObjectProxyModule(const char*);
void registerOscarModule(const char*);
// void registerOscarSecureModule(const char*);
void registerRPCService(const char*);
void registerSocketModule(const char*);
void registerTimerService(const char*);
void registerReferrerClientService(const char* pn);
static void registerModules()
{
    MUTEX_INSPECTOR;
    logErr2("static void registerModules() %s",_DMI().c_str());
    char *pn=NULL;
    {
#if !defined __MOBILE__
        registerHTTPModule(pn);
        registerTelnetService(pn);
#ifdef WEBDUMP
        registerWebHandlerModule(pn);
#endif
        registerErrorDispatcherService(pn);
#endif
        registerObjectProxyModule(pn);
        registerOscarModule(pn);
        // registerOscarSecureModule(pn);
        registerRPCService(pn);
        registerSocketModule(pn);
        registerTimerService(pn);
        registerReferrerClientService(pn);
    }
    void registerSSL(const char* pn);
    registerSSL(pn);
}

IUtils *Megatron::initMegatron(int argc, char** argv, const std::string &filesDir)
{
    std::string appName="app_name";
    if(argc>0)
    {
        appName.clear();
        for(int i=0; i<strlen(argv[0]); i++)
        {
            if(isalnum(argv[0][i]))
                appName+=argv[0][i];

        }
    }
    iUtils=new CUtils(argc,argv,appName);
#ifndef QT_CORE_LIB
//    if(filesDir.size()==0)
//        throw CommonError("if(filesDir.size()==0) %s",__PRETTY_FUNCTION__);
#endif
//    printf("Megatron::setFilesDir %s",filesDir.c_str());
#ifdef QT_CORE_LIB
    iUtils->setFilesDir(CACHE_TARGET_DIR);
#else
    iUtils->setFilesDir(filesDir);
#endif



    registerModules();
//    iUtils->regi
    return iUtils;
}
IInstance *Megatron::createInstance(const std::string& config)
{
    IInstance *instance=iUtils->createNewInstance("Megatron::createInstance");
    ConfigObj *cnf1=new ConfigObj("client",config);
    instance->setConfig(cnf1);
    instance->initServices();
    return instance;
}
//void Megatron::setFilesDir(IUtils* iu,const std::string &fdir)
//{
//}

