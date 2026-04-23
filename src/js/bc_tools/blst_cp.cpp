#include <iostream>
#include "blst_cp.h"

int main() {
    const uint8_t ikm1[] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    const uint8_t ikm2[] = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
    const uint8_t msg[]  = "Hello, BLS!";

    blst_cpp::SecretKey sk1(ikm1, sizeof(ikm1)-1);
    blst_cpp::SecretKey sk2(ikm2, sizeof(ikm2)-1);

    blst_cpp::PublicKey pk1(sk1);
    blst_cpp::PublicKey pk2(sk2);

    blst_cpp::Signature sig1(sk1, msg, sizeof(msg)-1);
    blst_cpp::Signature sig2(sk2, msg, sizeof(msg)-1);

    blst_cpp::AggregateSignature agg;
    agg.add(sig1);
    agg.add(sig2);

    std::vector<blst_cpp::PublicKey> pks = { pk1, pk2 };

    bool ok = blst_cpp::fast_aggregate_verify(pks, agg, msg, sizeof(msg)-1);
    std::cout << "Fast aggregate verify: " << std::boolalpha << ok << std::endl;
}
