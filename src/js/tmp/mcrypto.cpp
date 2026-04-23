#include <sodium.h>
#include <string>
#include "mcrypto.h"

std::string mcrypto::hash(const std::string & buf)
{
    unsigned char hash[crypto_generichash_BYTES];
    crypto_generichash(hash, sizeof(hash),
                       (unsigned char*)buf.data(), buf.size(),
                       NULL, 0); // без ключа
    return {(char*)hash, sizeof(hash)};
}
std::pair<std::string, std::string> mcrypto::generate_keypair()
{
    unsigned char pk[crypto_sign_PUBLICKEYBYTES];
    unsigned char sk[crypto_sign_SECRETKEYBYTES];
    crypto_sign_keypair(pk, sk);
    return {std::string((char*)pk, sizeof(pk)), std::string((char*)sk, sizeof(sk))};
}

std::string mcrypto::sign_detached(const std::string & message, const std::string & sk)
{
    std::string signature(crypto_sign_BYTES, 0);
    crypto_sign_detached((unsigned char*)signature.data(), NULL,
                         (unsigned char*)message.data(), message.size(),
                         (unsigned char*)sk.data());
    return signature;
}
bool mcrypto::verify_detached(const std::string & signature, const std::string & message, const std::string & pk)
{
    return crypto_sign_verify_detached((unsigned char*)signature.data(),
                                       (unsigned char*)message.data(), message.size(),
                                       (unsigned char*)pk.data()) == 0;
}
