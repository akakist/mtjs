#pragma once
#include <string>
#include "ioBuffer.h"
#include <bls/bls384_256.h>
#include <bls/bls.hpp>
// struct SignedBuffer
// {
//     std::string msg;
//     bls::Signature sign;
//     std::string signer;
// };
// inline outBuffer & operator<< (outBuffer& o, const SignedBuffer &b)
// {
//     o<<b.msg<<b.signer;
//     o<<iUtils->hex2bin(b.sign.serializeToHexStr());
//     return o;
// }
// inline inBuffer & operator>> (inBuffer& in, SignedBuffer &b)
// {
//     in>>b.msg>>b.signer;
//     std::string s;
//     in>>s;
//     b.sign.deserializeHexStr(iUtils->bin2hex(s));
//     return in;
// }
