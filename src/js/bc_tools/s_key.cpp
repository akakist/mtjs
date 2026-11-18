#define OPENSSL_SUPPRESS_DEPRECATED

#include "s_key.h"
#include "s_ed.h"
#include "commonError.h"
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>

static bool verify_ecdsa(int nid, const std::string& pk_bin,
                         const std::string& sig_bin, const std::string& msg)
{
    if (sig_bin.size() != 64) return false;
    EC_KEY* ec = EC_KEY_new_by_curve_name(nid);
    if (!ec) return false;
    const unsigned char* p = (const unsigned char*)pk_bin.data();
    if (!o2i_ECPublicKey(&ec, &p, (long)pk_bin.size())) { EC_KEY_free(ec); return false; }
    ECDSA_SIG* es = ECDSA_SIG_new();
    if (!es) { EC_KEY_free(ec); return false; }
    BIGNUM* r = BN_bin2bn((const unsigned char*)sig_bin.data(), 32, nullptr);
    BIGNUM* s = BN_bin2bn((const unsigned char*)sig_bin.data() + 32, 32, nullptr);
    ECDSA_SIG_set0(es, r, s);
    int ok = ECDSA_do_verify((const unsigned char*)msg.data(), (int)msg.size(), es, ec);
    ECDSA_SIG_free(es); EC_KEY_free(ec);
    return ok == 1;
}


static EC_KEY* ecdsa_key_from_private(int nid, const std::string& sk_bin)
{
    if (sk_bin.size() != 32)
        throw CommonError("ecdsa: bad private key size %d (must be 32)", (int)sk_bin.size());
    EC_KEY* ec = EC_KEY_new_by_curve_name(nid);
    if (!ec)
        throw CommonError("ecdsa: EC_KEY_new_by_curve_name failed");
    BIGNUM* priv = BN_bin2bn((const unsigned char*)sk_bin.data(), 32, nullptr);
    if (!priv || !EC_KEY_set_private_key(ec, priv))
    {
        if (priv) BN_free(priv);
        EC_KEY_free(ec);
        throw CommonError("ecdsa: set private key failed");
    }
    BN_free(priv);
    const EC_GROUP* g = EC_KEY_get0_group(ec);
    EC_POINT* pub = EC_POINT_new(g);
    if (!pub || !EC_POINT_mul(g, pub, EC_KEY_get0_private_key(ec), nullptr, nullptr, nullptr)
            || !EC_KEY_set_public_key(ec, pub))
    {
        if (pub) EC_POINT_free(pub);
        EC_KEY_free(ec);
        throw CommonError("ecdsa: derive public key failed");
    }
    EC_POINT_free(pub);
    return ec;
}

static std::string sign_ecdsa(int nid, const std::string& sk_bin, const std::string& msg)
{
    EC_KEY* ec = ecdsa_key_from_private(nid, sk_bin);
    ECDSA_SIG* es = ECDSA_do_sign((const unsigned char*)msg.data(), (int)msg.size(), ec);
    if (!es)
    {
        EC_KEY_free(ec);
        throw CommonError("ecdsa: ECDSA_do_sign failed");
    }
    // verify_ecdsa парсит r||s как два BIGNUM по 32 байта
    std::string out(64, '\0');
    BN_bn2binpad(ECDSA_SIG_get0_r(es), (unsigned char*)out.data(), 32);
    BN_bn2binpad(ECDSA_SIG_get0_s(es), (unsigned char*)out.data()+32, 32);
    ECDSA_SIG_free(es);
    EC_KEY_free(ec);
    return out;
}

static std::string public_ecdsa(int nid, const std::string& sk_bin, bool compressed)
{
    EC_KEY* ec = ecdsa_key_from_private(nid, sk_bin);
    EC_KEY_set_conv_form(ec, compressed ? POINT_CONVERSION_COMPRESSED : POINT_CONVERSION_UNCOMPRESSED);
    unsigned char* buf = nullptr;
    int len = i2o_ECPublicKey(ec, &buf);
    if (len <= 0 || !buf)
    {
        if (buf) OPENSSL_free(buf);
        EC_KEY_free(ec);
        throw CommonError("ecdsa: i2o_ECPublicKey failed");
    }
    std::string out((char*)buf, (size_t)len);
    OPENSSL_free(buf);
    EC_KEY_free(ec);
    return out;
}

bool verify(const Key& key, const std::string& sig_bin, const std::string& msg)
{
    switch (key.type) {
    case KEY_ED25519:   return verify_ed_pk(key.keybuffer, sig_bin, msg);
    case KEY_SECP256K1: return verify_ecdsa(NID_secp256k1, key.keybuffer, sig_bin, msg);
    case KEY_P256:      return verify_ecdsa(NID_X9_62_prime256v1, key.keybuffer, sig_bin, msg);
    default:            return false;
    }
}
std::string sign(const Key& key, const std::string& sk_bin, const std::string& msg)
{
    switch (key.type) {
    case KEY_ED25519:   return sign_ed(sk_bin, msg);
    case KEY_SECP256K1: return sign_ecdsa(NID_secp256k1, sk_bin, msg);
    case KEY_P256:      return sign_ecdsa(NID_X9_62_prime256v1, sk_bin, msg);
    default:            throw CommonError("sign: unknown key type %d", (int)key.type);
    }
}

std::string public_key(uint8_t type, const std::string& sk_bin, bool compressed)
{
    switch (type) {
    case KEY_ED25519:   return extract_public_ed(sk_bin);
    case KEY_SECP256K1: return public_ecdsa(NID_secp256k1, sk_bin, compressed);
    case KEY_P256:      return public_ecdsa(NID_X9_62_prime256v1, sk_bin, compressed);
    default:            throw CommonError("public_key: unknown key type %d", (int)type);
    }
}
