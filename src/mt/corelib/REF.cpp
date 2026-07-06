#include "REF.h"
#include "mutexable.h"
#include <cstdio>
#include <map>
#include <string>
#include "commonError.h"
#include "mutexInspector.h"


void throw_deref_nullptr()
{
    throw CommonError("dereferencing null pointer %s", _DMI().c_str());
}

#ifdef MEMLEAK_CHECK

static Mutex mx;
static int n = 0;
static std::map<std::string, int> mm;
void inc_ptr(const char *s)
{
    M_LOCK(mx);
    mm[s]++;
}
void dec_ptr(const char *s)
{
    M_LOCK(mx);
    mm[s]--;
}
int get_global_refcount()
{
    M_LOCK(mx);
    for (auto &z : mm)
    {
        if (z.second)
            printf("%s -> %d\n", z.first.c_str(), z.second);
    }
    return 0;
}

#endif