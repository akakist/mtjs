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


CommonError::CommonError(const std::string& str):m_error(str)
{
#ifdef DEBUG
    fprintf(stdout,"CommonError raised: %s %s\n",m_error.c_str(),_DMI().c_str());
#endif
}
CommonError::CommonError(const char* fmt, ...) :m_error(fmt)
{

    va_list ap;
    char str[1024*10];
    va_start(ap, fmt);
    vsnprintf(str, sizeof(str), fmt, ap);
    va_end(ap);
    m_error=str;
#ifdef DEBUG
    fprintf(stdout,"CommonError raised: %s %s\n",m_error.c_str(),_DMI().c_str());
#endif

}


void logErr2(const char* fmt, ...)
{

#ifdef WITH_SLOG
    auto prf=iUtils->getLogPrefix();
    std::string s_prf=iUtils->join(" @ ",prf);
#endif

    {
        va_list ap;
        va_start(ap, fmt);
#ifndef _WIN32
        if(1)
        {
            {
#ifdef WITH_SLOG
                fprintf(stdout," %s ",s_prf.c_str());
#endif
            }
            vfprintf(stdout,fmt, ap);
            fprintf(stdout,"\n");
#endif
        }

    }
}

