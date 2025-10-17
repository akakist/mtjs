#pragma once
#include <sys/types.h>
#include <stdint.h>
#include <vector>
#include <set>
#include <deque>
#include <string>
#include <map>
#include <list>
#include <string.h>
#include "refstring.h"

/**
* Utility to serialize/deserialize data
*/
/**
* Класс байтового потока без битовых операций, но с темплейтами STL
*/
class oBuf
{
public:
private:
    oBuf(const oBuf&);             // Not defined to prevent usage
    oBuf& operator=(const oBuf&);  // Not defined to prevent usage

    size_t bufsize;
    size_t cur_pos;
    char * buffer;

    // void adjust(size_t n);
    // void construct();

    oBuf& Pack$(const std::string& s );
    oBuf& put_PN$(const uint64_t &N);
    oBuf& put_8$(unsigned char c)
    {
        // buffer->container+=((char)c);
        if(cur_pos+1 > bufsize)
            throw std::bad_alloc();
        buffer[cur_pos]=c;
        cur_pos++;
        return *this;
    }

public:

    // REF_getter<refbuffer> buffer;

    oBuf(char *b, size_t sz): bufsize(sz),buffer(b),cur_pos(0) {}
    // ~oBuf();

    void clear();

    /// pack data
    oBuf& pack(const std::string& s);
    oBuf& pack(const char * s, size_t len);
    oBuf& pack(const void * s, size_t len);

    const unsigned char *data()const;
    unsigned char *const_data()const;

    /// get buffer
    REF_getter<refbuffer> asString() const;

    /// get buffer size
    size_t size() const;

    /// put uchar
    oBuf& put_8(unsigned char);

    /// put packed number
    oBuf& put_PN(const uint64_t);

    /// put packed number prefixed string
    oBuf& put_PSTR(const std::string &);


    oBuf& operator<<(const char*);
#ifdef __MACH__
#endif
    oBuf& operator<<(const std::string&);
    oBuf& operator<<(const bool&);
#ifdef _WIN32
    outBuffer& operator<<(const long&);
#endif
    oBuf& operator<<(const double&);
    oBuf& operator<<(const float&);
public:

    template <typename T,std::enable_if_t<std::is_arithmetic<T>::value, bool> = true >
    oBuf& operator<<(const T& v)
    {
        put_PN(v);
        return *this;
    }
};

template < class T > oBuf & operator << (oBuf& b,const std::vector < T > &v)
{
    b << (unsigned int)v.size();

    for (auto &j:v) b << j;

    return b;
}

template < class T > oBuf & operator << (oBuf& b,const std::list < T > &v)
{

    b << (unsigned int)v.size();
    for (auto  &j:v) b << j;
    return b;
}

template < class T > oBuf & operator << (oBuf& b,const std::deque < T > &v)
{

    b << (unsigned int)v.size();
    for (auto &j:v) b << j;
    return b;
}
template < class T > oBuf & operator << (oBuf& b,const std::set < T > &v)
{

    b << (unsigned int)v.size();
    for (auto &j: v) b << j;
    return b;
}

template < class T1, class T2 > oBuf & operator << (oBuf& b,const std::map < T1, T2 > &v)
{
    b << (unsigned int)v.size();
    for (auto j = v.begin(); j != v.end(); j++)
    {
        b << j->first << j->second;
    }
    return b;
}

template < class T1, class T2 > oBuf & operator << (oBuf& b,const std::pair < T1, T2 > &v)
{
    b << v.first<<v.second;
    return b;
}


oBuf & operator<< (oBuf& b,const REF_getter<refbuffer> &s);



