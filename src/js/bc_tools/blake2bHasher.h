#pragma once
#include <sodium.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include "commonError.h"
#include "THASH_id.h"
class Blake2bHasher {
public:
    // Конструктор: инициализация контекста
    Blake2bHasher(size_t outlen = 32, const unsigned char* key = nullptr, size_t keylen = 0)
        : outlen_(outlen)
    {
        if (outlen_ < crypto_generichash_BYTES_MIN || outlen_ > crypto_generichash_BYTES_MAX) {
            throw std::invalid_argument("Invalid output length for Blake2b");
        }
        if (crypto_generichash_init(&state_, key, keylen, outlen_) != 0) {
            throw std::runtime_error("crypto_generichash_init failed");
        }
    }

    // Запрещаем копирование (контекст уникален)
    Blake2bHasher(const Blake2bHasher&) = delete;
    Blake2bHasher& operator=(const Blake2bHasher&) = delete;

    // Разрешаем перемещение
    Blake2bHasher(Blake2bHasher&&) = default;
    Blake2bHasher& operator=(Blake2bHasher&&) = default;

    // Добавление данных кусками
    void update(const void* data, size_t len) {
        if (crypto_generichash_update(&state_, static_cast<const unsigned char*>(data), len) != 0) {
            throw std::runtime_error("crypto_generichash_update failed");
        }
    }
    void update(const std::string &s) {
        return update(s.data(),s.size());
    }

    // Финализация и получение результата в виде hex‑строки
    std::string final() {
        unsigned char out[outlen_];
        if (crypto_generichash_final(&state_, out, outlen_) != 0) {
            throw std::runtime_error("crypto_generichash_final failed");
        }
        return {(char*)out,outlen_};
        // std::ostringstream oss;
        // for (size_t i = 0; i < outlen_; i++) {
        //     oss << std::hex << std::setw(2) << std::setfill('0') << (int)out[i];
        // }
        // return oss.str();
    }

private:
    crypto_generichash_state state_;
    size_t outlen_;
};

inline THASH_id blake2b_hash(const std::string& data) {
    THASH_id out;
    out.container.resize(32);
    crypto_generichash_state state_;
    if(crypto_generichash((uint8_t*)out.container.data(), out.container.size(),
                          (uint8_t*)data.data(), data.size(),
                          NULL, 0))
    {
        throw CommonError("blake2b_hash %d",data.size());
    }
    return out;
}