#include "IUtils.h"
#include "commonError.h"
#include "mutex_inspector_entry.h"
#include "mutexInspector.h"
#ifdef _WIN32
#include "compat_win32.h"
#endif
std::string _DMI_()
{
#ifdef MUTEX_INSPECTOR_DEBUG
    // MUTEX_INSPECTOR;
    if (!iUtils) return std::string("");
    return iUtils->getIThreadNameController()->dump_mutex_inspector(pthread_self());
#endif
    return std::string("");
}
MutexInspector::MutexInspector(const char *ff, int ll, const char *func)
{
    mutex_inspector_entry e;
    e.f = ff;
    e.func=func;
    e.l = ll;
    e.t = time(NULL);
    if (!iUtils) throw CommonError("!iUtils");
    iUtils->getIThreadNameController()->push_mi(pthread_self(),&e);
}
MutexInspector::MutexInspector(const char *ff, int ll, const char *func, const std::string& ss)
{
    mutex_inspector_entry e;
    e.f = ff;
    e.func=func;
    e.l = ll;
    e.t = time(NULL);
    e.__s = ss;
    if (!iUtils) throw CommonError("!iUtils");
    iUtils->getIThreadNameController()->push_mi(pthread_self(),&e);
}
MutexInspector::~MutexInspector()
{
    if (!iUtils) {
        logErr2("!iUtils");
        return;
    }
    iUtils->getIThreadNameController()->pop_mi(pthread_self());
}

