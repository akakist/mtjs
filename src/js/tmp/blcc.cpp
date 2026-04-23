#include "blst.h"
#include <stdio.h>
#include <string.h>

// Стандартная схема: публичный ключ в G1, подпись в G2.
// Подпись: 96 байт после сериализации (G2), публичный ключ: 48 байт (G1).

int main() {
    const char *msg = "MerkleRoot123";
    size_t msg_len = strlen(msg);
    const char *DST = "BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_NUL_";

    // --- Генерация приватного ключа (примерный keygen от сидов) ---
    blst_scalar sk;
    unsigned char seed[32];
    memset(seed, 42, sizeof(seed));
    blst_keygen(&sk, seed, sizeof(seed));

    // --- Публичный ключ в G1 ---
    blst_p1 pk;
    blst_sk_to_pk_in_g1(&pk, &sk);

    // --- Хэширование сообщения в точку G2 ---
    blst_p2 h;
    blst_hash_to_g2(&h,
                    (const unsigned char*)msg, msg_len,
                    NULL, 0,
                    NULL, 0);

    // --- Подпись: sigma = H(m)^sk в G2 ---
    blst_p2 sig;
    blst_sign_pk_in_g1(&sig, &h, &sk);

    // --- Сериализация подписи (96 байт) ---
    unsigned char sig_bytes[96];
    blst_p2_serialize(sig_bytes, &sig);

    // --- Десериализация подписи обратно ---
    blst_p2 sig2;
    BLST_ERROR derr = blst_p2_deserialize(&sig2, sig_bytes);
    if (derr != BLST_SUCCESS) {
        printf("Ошибка десериализации подписи: %d\n", derr);
        return 1;
    }

    // --- Проверка подписи (внутри сам делает hash_to_g2) ---
    BLST_ERROR verr = blst_core_verify_pk_in_g1(&pk, &sig2,
                                                (const unsigned char*)msg, msg_len,
                                                DST, strlen(DST));
    if (verr == BLST_SUCCESS) {
        printf("Подпись корректна!\n");
    } else {
        printf("Ошибка проверки подписи: %d\n", verr);
        return 1;
    }

    // --- Дополнительно: сериализация публичного ключа (48 байт), если нужно ---
    unsigned char pk_bytes[48];
    blst_p1_serialize(pk_bytes, &pk);
    printf("PK (G1) сериализован: 48 байт, SIG (G2) сериализована: 96 байт\n");

    return 0;
}
