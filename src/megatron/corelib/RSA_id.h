#pragma once
#include <string>
#include "IUtils.h"
#include "ISSL.h"
#include "SHA1_id.h"
struct RSA_id
{
    friend bool operator < (const RSA_id& a,const RSA_id& b);
    friend bool operator == (const RSA_id& a,const RSA_id& b);
    friend bool operator != (const RSA_id& a,const RSA_id& b);
    friend outBuffer& operator<< (outBuffer& b,const RSA_id& s);
    friend inBuffer& operator>> (inBuffer& b,  RSA_id& s);
private:
    std::string container;
public:
    RSA_id() {}
    RSA_id(const std::string &n): container(n) {}
    void set(const std::string&s) {
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
    SHA1_id sha1() const
    {
        I_ssl *ssl=(I_ssl*)iUtils->queryIface(Ifaces::SSL);


        SHA1_id sha;
        sha.set(ssl->sha1(container));
        return sha;

    }

};
inline bool operator < (const RSA_id& a,const RSA_id& b)
{
    return a.container<b.container;
}
inline bool operator == (const RSA_id& a,const RSA_id& b)
{
    return a.container==b.container;
}
inline bool operator != (const RSA_id& a,const RSA_id& b)
{
    return a.container!=b.container;
}
inline outBuffer& operator<< (outBuffer& b,const RSA_id& s)
{
    b<<s.container;
    return b;
}
inline inBuffer& operator>> (inBuffer& b,  RSA_id& s)
{
    b>>s.container;
    return b;
}
