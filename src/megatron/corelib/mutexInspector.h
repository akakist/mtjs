#pragma once


/// tool to debug thread, display function call stack in mutex errors - dead locks etc
#include <string>
#include <IUtils.h>
class MutexInspector
{
public:
    MutexInspector(const char* ff, int ll, const char *func);
    MutexInspector(const char* ff, int ll, const char *func, const std::string& s);
    ~MutexInspector();
};
#ifdef MUTEX_INSPECTOR_DEBUG
#define MUTEX_INSPECTOR  MutexInspector fall12344(__FILE__,__LINE__,__PRETTY_FUNCTION__);
#define MUTEX_INSPECTORS(a)  MutexInspector fall123444(__FILE__,__LINE__,__PRETTY_FUNCTION__,a);
#define _DMI()  iUtils->getIThreadNameController()->dump_mutex_inspector(pthread_self())
#else
#define MUTEX_INSPECTOR
#define MUTEX_INSPECTORS(a)

#define _DMI() std::string("")

#endif

