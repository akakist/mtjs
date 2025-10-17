#include "commonError.h"
#include "IUtils.h"
#include "mutexInspector.h"
#include "mutexable.h"
#if !defined __ANDROID__ && !defined __FreeBSD__

#include <sys/timeb.h>
#endif
#ifndef _WIN32
#include <syslog.h>
#endif

#ifdef __MACH__
#ifndef __IOS__
#endif
#endif
#ifdef __ANDROID__
#include <android/log.h>
#endif
#include <stdarg.h>


#if !defined __MOBILE__
//static Mutex *__logLock=nullptr;
#endif
//bool prevLogUnlinked=false;
