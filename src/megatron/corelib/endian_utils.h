#ifndef ENDIAN_UTILS_HPP
#define ENDIAN_UTILS_HPP

#include <cstdint>
#include <cstring>
#include <algorithm>

namespace endian_utils {

// Проверка на little-endian (в compile-time)
    constexpr bool is_little_endian() {
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
        return __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__;
#else
        union {
            uint32_t i;
            uint8_t c[4];
        } bint = {0x01020304};
        return bint.c[0] == 0x04;
#endif
    }

// Преобразует double в network byte order (8 байт, big-endian)
    inline void htond(double value, uint8_t out[8]) {
        uint8_t temp[8];
        std::memcpy(temp, &value, 8);

        if (is_little_endian()) {
            std::reverse_copy(temp, temp + 8, out);
        } else {
            std::memcpy(out, temp, 8);
        }
    }

// Преобразует 8 байт из network byte order в double
    inline double ntohd(const uint8_t in[8]) {
        uint8_t temp[8];

        if (is_little_endian()) {
            std::reverse_copy(in, in + 8, temp);
        } else {
            std::memcpy(temp, in, 8);
        }

        double value;
        std::memcpy(&value, temp, 8);
        return value;
    }

} // namespace endian_utils

#endif // ENDIAN_UTILS_HPP
