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



