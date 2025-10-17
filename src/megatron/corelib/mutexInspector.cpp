#include "mutexInspector.h"
#include "IUtils.h"
#include "commonError.h"
#include "mutex_inspector_entry.h"
#ifdef _WIN32
#include "compat_win32.h"
#endif
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

