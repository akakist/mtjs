#include <iostream>
#include <bls/bls384_256.h>
#include <bls/bls.hpp>

int main() {
    std::cout << "sizeof(bls::PublicKey) = " << sizeof(bls::PublicKey) << " bytes\n";
    std::cout << "sizeof(bls::Signature) = " << sizeof(bls::Signature) << " bytes\n";
}