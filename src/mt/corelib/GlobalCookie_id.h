#pragma once
#ifdef DEBUG
#include <string>
#include "ioBuffer.h"


/**  std::string wrapper to add GlobalCookie_id type control on compiler level.
 To differ it from another essences, having same type.
 Used in DEBUG mode only. In release version it equal to wrapping type.
*/

struct GlobalCookie_id
{
    std::string container;
    GlobalCookie_id(const std::string &z): container(z) {}
    GlobalCookie_id() {}
};
inline bool operator < (const GlobalCookie_id& a,const GlobalCookie_id& b)
{
    if (a.container<b.container) return true;
    return false;
}
inline bool operator == (const GlobalCookie_id& a,const GlobalCookie_id& b)
{
    if (a.container==b.container)return true;
    return false;
}
inline bool operator != (const GlobalCookie_id& a,const GlobalCookie_id& b)
{
    if (a.container!=b.container)return true;
    return false;
}
inline outBuffer& operator<< (outBuffer& b,const GlobalCookie_id& s)
{
    b<<s.container;
    return b;
}
inline inBuffer& operator>> (inBuffer& b,  GlobalCookie_id& s)
{
    b>>s.container;
    return b;
}

#else
typedef std::string GlobalCookie_id;
#endif
