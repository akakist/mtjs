#pragma once
#include <sodium.h>
#include "mutexInspector.h"
#include "blake2bHasher.h"
inline bool verify_ed_pk(const std::string& pk, const std::string & signature, const std::string & msg)
{
    MUTEX_INSPECTOR;
    if(pk.size()!=crypto_sign_PUBLICKEYBYTES)
        throw CommonError("if(pk.size()!=crypto_sign_PUBLICKEYBYTES)");
    if(signature.size()!=crypto_sign_BYTES)
        throw CommonError("if(signature.size()!=crypto_sign_BYTES)");
    return crypto_sign_verify_detached((unsigned char*)signature.data(),
                                       reinterpret_cast<const unsigned char*>(msg.data()),
                                       msg.size(),
                                       (unsigned char*)pk.data())==0;

}
inline bool verify_ed_pk(const std::string& pk, const std::string & signature, const THASH_id & msg)
{
    MUTEX_INSPECTOR;
    if(pk.size()!=crypto_sign_PUBLICKEYBYTES)
        throw CommonError("if(pk.size()!=crypto_sign_PUBLICKEYBYTES)");
    if(signature.size()!=crypto_sign_BYTES)
        throw CommonError("if(signature.size()!=crypto_sign_BYTES)");
    return crypto_sign_verify_detached((unsigned char*)signature.data(),
                                       reinterpret_cast<const unsigned char*>(msg.container.data()),
                                       msg.container.size(),
                                       (unsigned char*)pk.data())==0;

}


inline std::string sign_ed(const std::string& sk, const std::string& msg)
{
    MUTEX_INSPECTOR;
    if(sk.size()!=crypto_sign_SECRETKEYBYTES)
    {
        logErr2("sk %s",sk.c_str());
        throw CommonError("if(sk.size()!=crypto_sign_SECRETKEYBYTES) %d %d",sk.size(),crypto_sign_SECRETKEYBYTES);

    }

    // auto hs=get_hash();
    std::string signature;
    signature.resize(crypto_sign_BYTES);
    // std::vector<unsigned char> sig(crypto_sign_BYTES);

    auto err=crypto_sign_detached((unsigned char*)signature.data(),
                                  nullptr,
                                  reinterpret_cast<const unsigned char*>(msg.data()),
                                  msg.size(),
                                  (unsigned char*)sk.data());
    if(err)
        logErr2("crypto_sign_detached failed");

    return signature;
    // std::string pk;
    // pk.resize(crypto_sign_ed25519_PUBLICKEYBYTES);
    //         crypto_sign_ed25519_sk_to_pk((uint8_t*)pk.data(), (unsigned char*)sk.data());

    //     auto err2 =crypto_sign_verify_detached((unsigned char*)signature.data(),(unsigned char*)msg.data(),msg.size(),(unsigned char*)pk.data());
    // if(err2)
    // {
    //     throw CommonError("crypto_sign_verify_detached failure 2");
    // }
}
