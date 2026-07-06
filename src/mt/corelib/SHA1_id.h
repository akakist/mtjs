#pragma once
#include <string>
#include "IUtils.h"
struct SHA1_id
{
    friend bool operator < (const SHA1_id& a,const SHA1_id& b);
    friend bool operator == (const SHA1_id& a,const SHA1_id& b);
    friend bool operator != (const SHA1_id& a,const SHA1_id& b);
    friend outBuffer& operator<< (outBuffer& b,const SHA1_id& s);
    friend inBuffer& operator>> (inBuffer& b,  SHA1_id& s);
private:
    std::string container;
public:
    SHA1_id() {}
    void set(const std::string& s) {
        container=s;
    }
    std::string get() {
        return container;
    }
    std::string toString() const
    {
        return container;
    }
    std::string hex() const
    {
        return iUtils->bin2hex(container);
    }

};

inline bool operator < (const SHA1_id& a,const SHA1_id& b)
{
    return a.container<b.container;
}
inline bool operator == (const SHA1_id& a,const SHA1_id& b)
{
    return a.container==b.container;
}
inline bool operator != (const SHA1_id& a,const SHA1_id& b)
{
    return a.container!=b.container;
}
inline outBuffer& operator<< (outBuffer& b,const SHA1_id& s)
{
    b<<s.container;
    return b;
}
inline inBuffer& operator>> (inBuffer& b,  SHA1_id& s)
{
    b>>s.container;
    return b;
}
