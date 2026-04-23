#include "blst.hpp" // Если используется официальный C++ wrapper

// Подписание
blst::SecretKey sk;
sk.keygen(ikm);
blst::P1 pk = sk.to_pk();
blst::P2Signature sig = sk.sign(message);

// Верификация
if (sig.verify(pk, message)) {
    // Успешно
}
