#ifdef KALL
#include <iostream>
#include "blst_cp.h"
#include <chrono>

#include <sodium.h>
#include <iostream>
#include <chrono>
#define mln 1000000.

double now_sec() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

int main_ed() {
    if (sodium_init() < 0) {
        std::cerr << "libsodium init failed\n";
        return 1;
    }

    // --- Generate keypair ---
    unsigned char pk[crypto_sign_PUBLICKEYBYTES];
    unsigned char sk[crypto_sign_SECRETKEYBYTES];
    crypto_sign_keypair(pk, sk);

    const unsigned char message[] = "hello world";
    const size_t message_len = sizeof(message) - 1;

    unsigned char signature[crypto_sign_BYTES];

    // --- Sign ---
    double t0 = now_sec();
    crypto_sign_detached(signature, nullptr, message, message_len, sk);
    double t1 = now_sec();

    std::cout << "Signing time ED25519 (mks): " << (t1 - t0)*mln << " mks\n";

    // --- Verify ---
    t0 = now_sec();
    int ok = crypto_sign_verify_detached(signature, message, message_len, pk);
    t1 = now_sec();

    std::cout << "Verification time ED25519 (mks): " << (t1 - t0) *mln << " mks\n";

    if (ok == 0)
        std::cout << "Signature OK\n";
    else
        std::cout << "Signature FAILED\n";

    return 0;
}


inline double now() { using clock = std::chrono::high_resolution_clock; return std::chrono::duration<double>(clock::now().time_since_epoch()).count(); }
int main() {
    bls::init();

    // Допустим, у нас есть seed-фраза или пароль
    std::string seedPhrase = "correct horse battery staple";

    // Секретный ключ из хэша seed-фразы
    bls::SecretKey sk;
    sk.setHashOf(seedPhrase.c_str(), seedPhrase.size());

    // Публичный ключ
    bls::PublicKey pk;
    sk.getPublicKey(pk);

    // Сообщение
    std::string msg = "Hello from seed phrase";

    // Подпись
    bls::Signature sig;
    auto t1=now();
    for(int i=0;i< 10000;i++)
    {
    sk.sign(sig, msg);

    }
    auto t2=now();
    auto dt=t2-t1;
    std::cout << "Signing time for BLS signatures (mks): " << dt/10000. * mln << " mks" << std::endl;
    t1=now();
    for(int i=0;i< 10000;i++)
    {

        // Проверка
        if (sig.verify(pk, msg)) {
            // std::cout << "Signature verified!" << std::endl;
        } else {
            std::cout << "Verification failed!" << std::endl;
        }
    }
    t2=now();
    dt=t2-t1;
    std::cout << "Verification time for BLS signatures (mks): " << dt/10000. * mln << " mks" << std::endl;

    main_ed();
    return 0;
}
#endif
int main() {
    // std::cout << "This test is for benchmarking BLS and Ed25519 signatures. Please define KALL to run it.\n";
    return 0;
}