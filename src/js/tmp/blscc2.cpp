#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "blst.h"

// Вспомогательная функция: печать 96-байтового скаляра/подписи в hex
void print_hex(const char* label, const void* data, size_t len) {
    printf("%s (%zu байт): ", label, len);
    for (size_t i = 0; i < len; i++)
        printf("%02x", ((const unsigned char*)data)[i]);
    printf("\n");
}

int main() {
    blst_scalar_from_le_bytes
    // ------------------------------------------------------------------
    // 1. Генерация секретного ключа (IKM ≥ 32 байт)
    // ------------------------------------------------------------------
    const char *ikm = "this-is-a-very-secret-32-byte-seed!!!!!!"; // 40 байт — ок
    blst_scalar sk_scalar;
    blst_scalar_from_lendian(&sk_scalar, (byte*)ikm);           // простейший способ
    // Лучше так (рекомендовано в RFC 9380):
    blst_keygen(&sk_scalar, (const byte*)ikm, strlen(ikm), NULL, 0);

    // Секретный ключ в виде 32-байтового массива
    byte sk_bytes[32];
    blst_lendian_from_scalar(sk_bytes, &sk_scalar);

    print_hex("Secret key", sk_bytes, 32);

    // ------------------------------------------------------------------
    // 2. Публичный ключ (в G2)
    // ------------------------------------------------------------------
    blst_p2 pk;
    blst_sk_to_pk_in_g2(&pk, &sk_scalar);   // sk → pk в G2

    byte pk_bytes[96];
    blst_p2_compress(pk_bytes, &pk);
    print_hex("Public key (compressed)", pk_bytes, 96);

    // ------------------------------------------------------------------
    // 3. Сообщение, которое подписываем
    // ------------------------------------------------------------------
    const char *msg = "Привет, это сообщение для подписи BLS!";
    size_t msg_len = strlen(msg);

    // ------------------------------------------------------------------
    // 4. Подпись сообщения (схема BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_POP_)
    // ------------------------------------------------------------------
    blst_p1 signature;
    blst_sign_pk2_in_g1(&signature, &sk_scalar, msg, msg_len);

    byte sig_bytes[96];
    blst_p1_compress(sig_bytes, &signature);
    print_hex("Signature (96 байт)", sig_bytes, 96);

    // ------------------------------------------------------------------
    // 5. Проверка подписи
    // ------------------------------------------------------------------
    int ok = blst_verify_pk2_in_g1(&pk, sig_bytes, msg, msg_len);
    printf("Подпись валидна? %s\n", ok == BLST_SUCCESS ? "ДА" : "НЕТ");

    // ------------------------------------------------------------------
    // 6. Проверка с изменённым сообщением → должна упасть
    // ------------------------------------------------------------------
    const char *bad_msg = "Привет, это сообщение для подписи BLS! (изменено)";
    int bad = blst_verify_pk2_in_g1(&pk, sig_bytes, bad_msg, strlen(bad_msg));
    printf("Подпись на изменённом сообщении валидна? %s\n",
           bad == BLST_SUCCESS ? "ДА (ошибка!)" : "НЕТ (правильно)");

    return 0;
}