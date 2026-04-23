#include <bls/bls384_256.h>
#include <bls/bls.hpp>
#include <iostream>
#include <vector>

int main() {
    bls::init();

    const char *msg = "hello world";
    size_t msg_len = strlen(msg);

    // --- Генерация ключей ---
    const int N = 3;
    std::vector<bls::SecretKey> sks(N);
    std::vector<bls::PublicKey> pks(N);
    for (int i = 0; i < N; i++) {
        sks[i].init();
        std::string seed = "seed phrase"+std::to_string(i);
        bls::SecretKey sk;
        sk.setHashOf(seed.data(), seed.size());
        sks[i].getPublicKey(pks[i]);
    }

    // --- Подписи ---
    std::vector<bls::Signature> sigs(N);
    for (int i = 0; i < N; i++) {
        sks[i].sign(sigs[i], msg, msg_len);
    }

    // --- Агрегация подписей ---
    bls::Signature aggSig = sigs[0];
    for (int i = 1; i < N; i++) {
        aggSig.add(sigs[i]);
    }


    // --- Агрегация публичных ключей ---
    bls::PublicKey aggPk = pks[0];
    for (int i = 1; i < N; i++) {
        aggPk.add(pks[i]);
    }

    // --- Проверка: подпись против агрегированного публичного ключа ---
    bool ok = aggSig.verify(aggPk, msg, msg_len);
    auto s=aggSig.serializeToHexStr();
    std::cout << (ok ? "Агрегированная подпись корректна!" : "Ошибка проверки")<< std::endl << s << std::endl;

    return 0;
}
