#pragma once

#include <exception>
#include <stdexcept>
#include <string>
#include <stdarg.h>
#include "mutexInspector.h"
/**
* Common exception
*/

class CommonError: public std::exception
{
private:
    std::string m_error;
public:
    void append(const std::string &s)
    {
        m_error.append(s);
    }
    explicit CommonError(const std::string& str);
    explicit CommonError(const char* fmt, ...);
    virtual ~CommonError() {}
    const char* what() const  noexcept
    {
        return m_error.c_str();
    };
};
void logErr2(const char* fmt, ...);
// void logRemote(const char *fmt, ...);


/// DBG - hide code in release mode
#ifdef DEBUG
#define DBG(a) a
#else
#define DBG(a)
#endif

/// XTRY, XPASS is two macros used to print stack while exception throwing
#ifdef MUTEX_INSPECTOR_DEBUG
#define XTRY try{
#define XPASS } catch(...){logErr2("XPASS @%s %s %d",__func__,__FILE__,__LINE__);throw;}
#define XPASS_S(s) } catch(...){logErr2("XPASS %s @%s %s %d",s.c_str(),__func__,__FILE__,__LINE__);throw;}
#else
#define XTRY
#define XPASS
#define XPASS_S
#endif


inline CommonError::CommonError(const std::string& str):m_error(str)
{
#ifdef DEBUG
    fprintf(stderr,"CommonError raised: %s %s\n",m_error.c_str(),_DMI().c_str());
#endif
}
inline CommonError::CommonError(const char* fmt, ...) :m_error(fmt)
{

    va_list ap;
    char str[1024*10];
    va_start(ap, fmt);
    vsnprintf(str, sizeof(str), fmt, ap);
    va_end(ap);
    m_error=str;
#ifdef DEBUG
    fprintf(stderr,"CommonError raised: %s %s\n",m_error.c_str(),_DMI().c_str());
#endif

}

#if !defined __MOBILE__

// static std::string getLogName()
// {
//     if(!iUtils) return ".log";
//     std::string fn=(std::string)iUtils->app_name()+".log";
//     {
//         fn=iUtils->gLogDir()+"/"+fn;
//     }
//     if(!prevLogUnlinked)
//     {
//         prevLogUnlinked=true;
//     }
//     return fn;
// }
#else
#endif

inline void logErr2(const char* fmt, ...)
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
//                fprintf(stderr,"%s: ",iUtils->app_name().c_str());
#ifdef WITH_SLOG
                fprintf(stderr," %s ",s_prf.c_str());
#endif
            }
            vfprintf(stderr,fmt, ap);
            fprintf(stderr,"\n");
#endif
        }

    }
}


