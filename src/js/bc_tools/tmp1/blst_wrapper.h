// blst_wrapper.hpp
#ifndef BLST_WRAPPER_HPP
#define BLST_WRAPPER_HPP

#include <vector>
#include <array>
#include <string>
#include <memory>
#include <stdexcept>
#include <optional>

// Подключаем заголовки blst
extern "C" {
#include "blst.h"
#include "blst_aux.h"
}

namespace blst {

// Исключение для ошибок библиотеки
    class BlstException : public std::runtime_error {
    public:
        explicit BlstException(const std::string& msg) : std::runtime_error(msg) {}
    };

// Типы для ключей и подписей (сжатые форматы)
    using SecretKey = std::array<uint8_t, 32>;
    using PublicKey = std::array<uint8_t, 48>;
    using Signature = std::array<uint8_t, 96>;

// Домен разделения (DST) для hash-to-curve
    const std::string BLS_DST = "BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_NUL_";

// ============================================================================
// Вспомогательные функции
// ============================================================================

    inline void check_error(BLST_ERROR err) {
        if (err != BLST_SUCCESS) {
            throw BlstException("BLST error: " + std::to_string(err));
        }
    }

// Генерация случайного скаляра (секретного ключа)
// Внимание: требует криптостойкий RNG!
    SecretKey generate_secret_key(const uint8_t* entropy, size_t entropy_size) {
        SecretKey sk{};
        blst_scalar scalar;
        blst_keygen(&scalar, entropy, entropy_size);
        blst_scalar_to_bendian(sk.data(), &scalar);
        return sk;
    }

// ============================================================================
// RAII обёртки для C-типов
// ============================================================================

// Обёртка для blst_p1 (точка на G1 - публичные ключи)
    class P1Point {
    private:
        struct Free {
            void operator()(blst_p1* p) const {
                if (p) free(p);
            }
        };
        std::unique_ptr<blst_p1, Free> ptr_;

    public:
        P1Point() : ptr_(new blst_p1, Free{}) {}
        explicit P1Point(blst_p1* p) : ptr_(p, Free{}) {}

        blst_p1* get() {
            return ptr_.get();
        }
        const blst_p1* get() const {
            return ptr_.get();
        }

        // Сериализация в сжатый формат (48 байт)
        PublicKey serialize() const {
            PublicKey out{};
            blst_p1_compress(out.data(), ptr_.get());
            return out;
        }

        // Десериализация с проверкой принадлежности группе
        static P1Point deserialize(const PublicKey& data) {
            P1Point point;
            BLST_ERROR err = blst_p1_uncompress(point.ptr_.get(), data.data());
            check_error(err);

            // Групповая проверка (важно для безопасности!)
            if (!blst_p1_in_g1(point.ptr_.get())) {
                throw BlstException("Point not in G1");
            }
            return point;
        }

        // Публичный ключ из секретного
        static P1Point from_secret_key(const SecretKey& sk) {
            P1Point point;
            blst_scalar scalar;
            blst_scalar_from_bendian(&scalar, sk.data());
            blst_sk_to_pk_in_g1(point.ptr_.get(), &scalar);
            return point;
        }
    };

// Обёртка для blst_p2 (точка на G2 - подписи)
    class P2Point {
    private:
        struct Free {
            void operator()(blst_p2* p) const {
                if (p) free(p);
            }
        };
        std::unique_ptr<blst_p2, Free> ptr_;

    public:
        P2Point() : ptr_(new blst_p2, Free{}) {}

        blst_p2* get() {
            return ptr_.get();
        }
        const blst_p2* get() const {
            return ptr_.get();
        }

        // Сериализация в сжатый формат (96 байт)
        Signature serialize() const {
            Signature out{};
            blst_p2_compress(out.data(), ptr_.get());
            return out;
        }

        // Десериализация с проверкой принадлежности группе (внутренняя проверка)
        static P2Point deserialize(const Signature& data) {
            P2Point point;
            BLST_ERROR err = blst_p2_uncompress(point.ptr_.get(), data.data());
            check_error(err);
            // blst сам проверяет подписи при верификации
            return point;
        }

        // Подпись сообщения
        static P2Point sign(const SecretKey& sk, const std::vector<uint8_t>& message) {
            // Хэшируем сообщение в точку G2
            P2Point hashed_point;
            const uint8_t* dst = reinterpret_cast<const uint8_t*>(BLS_DST.data());
            blst_hash_to_g2(hashed_point.ptr_.get(), message.data(), message.size(),
                            dst, BLS_DST.size(), nullptr, 0);

            // Умножаем на секретный ключ
            P2Point signature;
            blst_scalar scalar;
            blst_scalar_from_bendian(&scalar, sk.data());
            blst_sign_pk_in_g2(signature.ptr_.get(), hashed_point.ptr_.get(), &scalar);

            return signature;
        }
    };

// ============================================================================
// Высокоуровневый интерфейс
// ============================================================================

    class BlstCrypto {
    public:
        // Генерация пары ключей
        static std::pair<SecretKey, PublicKey> generate_keypair(const uint8_t* entropy, size_t entropy_size) {
            SecretKey sk = generate_secret_key(entropy, entropy_size);
            P1Point pk_point = P1Point::from_secret_key(sk);
            return {sk, pk_point.serialize()};
        }

        // Подпись сообщения
        static Signature sign(const SecretKey& sk, const std::vector<uint8_t>& message) {
            return P2Point::sign(sk, message).serialize();
        }

        // Верификация одной подписи
        static bool verify(const PublicKey& pk, const Signature& sig, const std::vector<uint8_t>& message) {
            try {
                // Десериализуем ключ и подпись
                P1Point pk_point = P1Point::deserialize(pk);
                P2Point sig_point = P2Point::deserialize(sig);

                // Хэшируем сообщение в G2
                P2Point hashed_point;
                const uint8_t* dst = reinterpret_cast<const uint8_t*>(BLS_DST.data());
                blst_hash_to_g2(hashed_point.get(), message.data(), message.size(),
                                dst, BLS_DST.size(), nullptr, 0);

                // Создаём контекст спаривания
                std::vector<uint8_t> pairing_buf(blst_pairing_sizeof());
                blst_pairing* ctx = reinterpret_cast<blst_pairing*>(pairing_buf.data());

                // Инициализация
                blst_pairing_init(ctx, BLST_HASH_OR_ENCODE, dst, BLS_DST.size());

                // Добавляем пару (публичный ключ, сообщение)
                blst_pairing_aggregate_pk_in_g1(ctx, pk_point.get(), sig_point.get(),
                                                hashed_point.get());

                // Финализация и проверка
                blst_pairing_commit(ctx);
                return blst_pairing_finalverify(ctx, nullptr) != 0;

            } catch (const BlstException&) {
                return false;
            }
        }

        // Агрегация нескольких подписей
        static Signature aggregate_signatures(const std::vector<Signature>& signatures) {
            if (signatures.empty()) {
                throw BlstException("No signatures to aggregate");
            }

            P2Point aggregated;
            // Инициализация бесконечной точкой
            blst_p2_from_affine(aggregated.get(), nullptr);

            for (const auto& sig : signatures) {
                P2Point sig_point = P2Point::deserialize(sig);
                blst_p2_add_or_double_affine(aggregated.get(), aggregated.get(), sig_point.get());
            }

            return aggregated.serialize();
        }

        // Верификация агрегированной подписи
        static bool aggregate_verify(const std::vector<PublicKey>& public_keys,
                                     const std::vector<std::vector<uint8_t>>& messages,
                                     const Signature& aggregated_sig) {
            if (public_keys.size() != messages.size() || public_keys.empty()) {
                return false;
            }

            try {
                P2Point agg_sig = P2Point::deserialize(aggregated_sig);

                std::vector<uint8_t> pairing_buf(blst_pairing_sizeof());
                blst_pairing* ctx = reinterpret_cast<blst_pairing*>(pairing_buf.data());

                const uint8_t* dst = reinterpret_cast<const uint8_t*>(BLS_DST.data());
                blst_pairing_init(ctx, BLST_HASH_OR_ENCODE, dst, BLS_DST.size());

                for (size_t i = 0; i < public_keys.size(); ++i) {
                    P1Point pk_point = P1Point::deserialize(public_keys[i]);

                    // Хэшируем каждое сообщение
                    P2Point hashed;
                    blst_hash_to_g2(hashed.get(), messages[i].data(), messages[i].size(),
                                    dst, BLS_DST.size(), nullptr, 0);

                    // Добавляем пару (ключ, сообщение)
                    BLST_ERROR err = blst_pairing_aggregate_pk_in_g1(ctx, pk_point.get(),
                                     nullptr, hashed.get());
                    if (err != BLST_SUCCESS) return false;
                }

                blst_pairing_commit(ctx);
                return blst_pairing_finalverify(ctx, agg_sig.get()) != 0;

            } catch (const BlstException&) {
                return false;
            }
        }
    };

} // namespace blst

#endif // BLST_WRAPPER_HPP