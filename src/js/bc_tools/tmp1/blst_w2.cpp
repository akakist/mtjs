#include "blst_w2.h"
#include <iostream>

int main() {
    const uint8_t msg[] = "hello";
    const uint8_t dst[] = "BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_NUL_";

    blst_cpp::SecretKey sk1((uint8_t*)"a", 1);
    blst_cpp::SecretKey sk2((uint8_t*)"b", 1);

    blst_cpp::PublicKey pk1(sk1);
    blst_cpp::PublicKey pk2(sk2);

    blst_cpp::Signature s1(sk1, msg, 5, dst, sizeof(dst)-1);
    blst_cpp::Signature s2(sk2, msg, 5, dst, sizeof(dst)-1);

    blst_cpp::AggregateSignature agg;
    agg.add(s1);
    agg.add(s2);

    bool ok = blst_cpp::aggregate_verify(
        agg,
        {pk1, pk2},
        msg,
        5,
        dst,
        sizeof(dst)-1
    );

    std::cout << ok << std::endl;
}