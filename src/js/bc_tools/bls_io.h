#ifdef KALL
#pragma once
#include <ioBuffer.h>
#include <blstpp.h>
#include "IUtils.h"

inline outBuffer& operator<<(outBuffer &o,const bls::SecretKey& z)
{
    o<<iUtils->hex2bin(z.serializeToHexStr());
    return o;
}
inline inBuffer& operator>>(inBuffer &o,bls::SecretKey& z)
{
    std::string s;
    o>>s;
    z.deserializeHexStr(iUtils->bin2hex(s));
    return o;
}
inline outBuffer& operator<<(outBuffer &o,const bls::PublicKey& z)
{
    o<<iUtils->hex2bin(z.serializeToHexStr());
    return o;
}
inline inBuffer& operator>>(inBuffer &o,bls::PublicKey& z)
{
    std::string s;
    o>>s;
    z.deserializeHexStr(iUtils->bin2hex(s));
    return o;
}
inline outBuffer& operator<<(outBuffer &o,const bls::Signature& z)
{
    o<<iUtils->hex2bin(z.serializeToHexStr());
    return o;
}
inline inBuffer& operator>>(inBuffer &o,bls::Signature& z)
{
    std::string s;
    o>>s;
    z.deserializeHexStr(iUtils->bin2hex(s));
    return o;
}
#endif