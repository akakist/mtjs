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
class inBuffer
{
public:


private:
    size_t out_pos;
    size_t m_size;
    const unsigned char *m_data;
public:
    ~inBuffer() {};
    inBuffer(const unsigned char *d, size_t size); // construct from an array
    inBuffer(const  char *d, size_t size); // construct from an array
    inBuffer(const  std::string&); // construct from string

    inBuffer(const  REF_getter<refbuffer>&); // construct from string

    /// full size buffer
    size_t size() const;

    /// pointer to buffer
    const unsigned char *data();

    /// remainder bytes count
    size_t remains() const;

    /// inicate reminder size > 0
    bool beforeEnd() const;



public:
    /// get reminder to string
    std::string unpackRest();

    /// get 'size' bytes into 's', throws if reminder < size
    void unpack(std::string& s, int64_t size);

    /// get 'size' bytes into 'buf', throws if reminder < size
    void unpack(void* buf, size_t sz);

    /// get 'size' bytes into 's', nothrows, success indicate that buffer enough
    void unpack_nothrow(std::string& s, size_t size, bool &success);



    /// get 1 byte, throws
    unsigned char get_8();

    /// get 1 byte, nothrows
    unsigned char get_8_nothrow(bool &success);

    /// get packed number, throws
    uint64_t get_PN();

    /// get packed number, nothrows
    uint64_t get_PN_nothrow(bool &success);

    /// get packed number prefixed string (PN - string lenth), throws
    std::string get_PSTR();

    /// get packed number prefixed string (PN - string lenth), nothrows
    std::string get_PSTR_nothrow(bool &success);

#ifdef _WIN32
//    inBuffer& operator>>(long &);
#endif

#ifdef __MACH__
#endif
    inBuffer& operator>>(std::string&);
    inBuffer& operator>>(bool&);
    inBuffer& operator>>(double&);
    inBuffer& operator>>(float &);
public:
    template <typename T,std::enable_if_t<std::is_arithmetic<T>::value, bool> = true >
    inBuffer& operator>>(T &v)
    {
        v=get_PN();
        return *this;
    }
};
/**
* Класс байтового потока без битовых операций, но с темплейтами STL
*/
class outBuffer
{
public:
private:
    outBuffer(const outBuffer&);             // Not defined to prevent usage
    outBuffer& operator=(const outBuffer&);  // Not defined to prevent usage

    // size_t bufsize;
    // size_t cur_pos;

    // void adjust(size_t n);
    // void construct();

    outBuffer& Pack$(const std::string& s );
    outBuffer& put_PN$(const uint64_t &N);
    outBuffer& put_8$(unsigned char c)
    {
        buffer->container+=((char)c);
        return *this;
    }

public:

    REF_getter<refbuffer> buffer;

    outBuffer();
    ~outBuffer();

    void clear();

    /// pack data
    outBuffer& pack(const std::string& s);
    outBuffer& pack(const char * s, size_t len);
    outBuffer& pack(const void * s, size_t len);

    const unsigned char *data()const;
    unsigned char *const_data()const;

    /// get buffer
    REF_getter<refbuffer> asString() const;

    /// get buffer size
    size_t size() const;

    /// put uchar
    outBuffer& put_8(unsigned char);

    /// put packed number
    outBuffer& put_PN(const uint64_t);

    /// put packed number prefixed string
    outBuffer& put_PSTR(const std::string &);


    outBuffer& operator<<(const char*);
#ifdef __MACH__
#endif
    outBuffer& operator<<(const std::string&);
    outBuffer& operator<<(const bool&);
#ifdef _WIN32
    outBuffer& operator<<(const long&);
#endif
    outBuffer& operator<<(const double&);
    outBuffer& operator<<(const float&);
public:

    template <typename T,std::enable_if_t<std::is_arithmetic<T>::value, bool> = true >
    outBuffer& operator<<(const T& v)
    {
        put_PN(v);
        return *this;
    }
};

template < class T > outBuffer & operator << (outBuffer& b,const std::vector < T > &v)
{
    b << (unsigned int)v.size();

    for (auto &j:v) b << j;

    return b;
}

template < class T > outBuffer & operator << (outBuffer& b,const std::list < T > &v)
{

    b << (unsigned int)v.size();
    for (auto  &j:v) b << j;
    return b;
}

template < class T > outBuffer & operator << (outBuffer& b,const std::deque < T > &v)
{

    b << (unsigned int)v.size();
    for (auto &j:v) b << j;
    return b;
}
template < class T > outBuffer & operator << (outBuffer& b,const std::set < T > &v)
{

    b << (unsigned int)v.size();
    for (auto &j: v) b << j;
    return b;
}

template < class T1, class T2 > outBuffer & operator << (outBuffer& b,const std::map < T1, T2 > &v)
{
    b << (unsigned int)v.size();
    for (auto j = v.begin(); j != v.end(); j++)
    {
        b << j->first << j->second;
    }
    return b;
}

template < class T1, class T2 > outBuffer & operator << (outBuffer& b,const std::pair < T1, T2 > &v)
{
    b << v.first<<v.second;
    return b;
}


template < class T > inBuffer & operator >> (inBuffer& b,std::set < T > &v)
{
    v.clear();
    unsigned int n;
    b >> n;
    for (unsigned int j = 0; j < n; j++)
    {
        T t;
        b >> t;
        v.insert(std::move(t));
    }
    return b;
}
template < class T > inBuffer& operator >> (inBuffer& b,std::list < T > &v)
{
    v.clear();
    unsigned int n;
    b >> n;
    for (unsigned int j = 0; j < n; j++)
    {
        T t;
        b >> t;
        v.push_back(std::move(t));
    }
    return b;
}
template < class T1, class T2 > inBuffer & operator >> (inBuffer& b,std::map < T1, T2 > &v)
{
    v.clear();
    unsigned int n;
    b >> n;
    for (unsigned int j = 0; j < n; j++)
    {
        T1 t1;
        T2 t2;
        b >> t1;
        b >> t2;
        v.insert(std::move(std::pair<T1,T2>(std::move(t1),std::move(t2))));
    }
    return b;
}
template < class T1, class T2 > inBuffer & operator >> (inBuffer& b,std::pair < T1, T2 > &v)
{
    b >> v.first;
    b >> v.second;
    return b;
}
template < class T > inBuffer & operator >> (inBuffer& b,std::vector < T > &v)
{
    v.clear();
    unsigned int n;
    b >> n;
    v.reserve(n);
    for (unsigned int j = 0; j < n; j++)
    {
        T t;
        b >> t;
        v.push_back(std::move(t));
    }
    return b;
}

template < class T > inBuffer & operator >> (inBuffer& b,std::deque < T > &v)
{
    v.clear();
    unsigned int n;
    b >> n;
    for (unsigned int j = 0; j < n; j++)
    {
        T t;
        b >> t;
        v.push_back(std::move(t));
    }
    return b;
}

inline unsigned char inBuffer::get_8_nothrow(bool &success)
{
    if (out_pos + 1 > m_size)
    {
        success=false;
        return 0;
    }
    success=true;
    unsigned char c= m_data[out_pos++];
    return c;
}
inline uint64_t inBuffer::get_PN()
{
    uint64_t NN=0;
    while (1)
    {

        unsigned char c=get_8();
        if (c<0x80)
        {
            NN<<=7;
            NN|=c;
            return NN;
        }
        else
        {
            NN<<=7;
            NN|=c&0x7F;

        }
    }
}
inline uint64_t inBuffer::get_PN_nothrow(bool &success)
{
    uint64_t NN=0;
    while (1)
    {
        unsigned char c=get_8_nothrow(success);
        if (!success)return 0;
        if (c<0x80)
        {
            NN<<=7;
            NN|=c;
            return NN;
        }
        else
        {
            NN<<=7;
            NN|=c&0x7F;

        }
    }
}
outBuffer & operator<< (outBuffer& b,const REF_getter<refbuffer> &s);
inBuffer & operator>> (inBuffer& b,  REF_getter<refbuffer> &s);



