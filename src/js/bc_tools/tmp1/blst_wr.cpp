#include "blst_wr.h"
#include <iostream>

int main() {
    const uint8_t ikm[] = {1,2,3,4,5};

    const uint8_t msg[] = "hello";
    const uint8_t dst[] = "BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_NUL_";

    blst_cpp::SecretKey sk(ikm, sizeof(ikm));
    blst_cpp::PublicKey pk(sk);

    blst_cpp::Signature sig(sk, msg, sizeof(msg)-1, dst, sizeof(dst)-1);

    bool ok = sig.verify(pk, msg, sizeof(msg)-1, dst, sizeof(dst)-1);

    std::cout << "verify: " << ok << std::endl;
}