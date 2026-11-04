#define OPENSSL_SUPPRESS_DEPRECATED

#include "s_key.h"
#include "s_ed.h"
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

bool verify(const Key& key, const std::string& sig_bin, const std::string& msg)
{
    switch (key.type) {
    case KEY_ED25519:   return verify_ed_pk(key.keybuffer, sig_bin, msg);
    case KEY_SECP256K1: return verify_ecdsa(NID_secp256k1, key.keybuffer, sig_bin, msg);
    case KEY_P256:      return verify_ecdsa(NID_X9_62_prime256v1, key.keybuffer, sig_bin, msg);
    default:            return false;
    }
}