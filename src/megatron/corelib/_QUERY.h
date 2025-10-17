#pragma once


#include <string>
#include <deque>
#include "Real.h"

#define MAX_PARAMS_IN_SQL		64
/**
 Object to create SQL request
 usage:
 QUERY q=(QUERY)"select * from a where q=? w=? r=?"<<q<<w<<r;
*/

class QUERY
{
public:
    QUERY(const std::string &);
    QUERY(const char* s);


    virtual ~QUERY();

    std::deque<std::string> params_;
    std::string query_;
    std::string prepare() const;

    QUERY& operator << (const std::string &);
    QUERY& operator << (const int8_t);
    QUERY& operator << (const uint8_t);
    QUERY& operator << (const int16_t);
    QUERY& operator << (const uint16_t);
    QUERY& operator << (const int32_t);
    QUERY& operator << (const uint32_t);
    QUERY& operator << (const int64_t);
    QUERY& operator << (const uint64_t);
#ifndef _LP64
#if !defined(_WIN32) && !defined(__MACH__)
    QUERY& operator << (const time_t&);
#endif
#endif
private:
    QUERY& operator << (const real);
public:
#if defined(_WIN32) || defined(__MACH__)
    QUERY& operator << (const long);
#endif

};


