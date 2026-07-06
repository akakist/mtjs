#pragma once
#include <openssl/bn.h>
#include <stdexcept>
#include <string>
#include <iostream>
#include "mutexInspector.h"

class BigInt {

    BIGNUM* bn;

    void ensureAlloc() {
        if (!bn) {
            bn = BN_new();
            if (!bn) throw std::runtime_error("BN_new failed");
        }
    }

public:
    friend outBuffer& operator<<(outBuffer &o,const BigInt& z);
    friend inBuffer& operator>>(inBuffer &o,BigInt& z);
    // Конструкторы
    BigInt() : bn(BN_new()) {
        if (!bn) throw std::runtime_error("BN_new failed");
    }

    explicit BigInt(const std::string& dec) : BigInt() {
        if (!BN_dec2bn(&bn, dec.c_str()))
            throw std::runtime_error("BN_dec2bn failed  "+dec+" "+_DMI());
    }
    BigInt & from_string(const std::string& dec)  {
        if (!BN_dec2bn(&bn, dec.c_str()))
            throw std::runtime_error("BN_dec2bn failed  "+dec+" "+_DMI());

        return *this;
    }

    BigInt(const BigInt& other) {
        bn = BN_dup(other.bn);
        if (!bn) throw std::runtime_error("BN_dup failed");
    }

    BigInt(BigInt&& other) noexcept : bn(other.bn) {
        other.bn = nullptr;
    }

    BigInt& operator=(const BigInt& other) {
        if (this != &other) {
            BN_free(bn);
            bn = BN_dup(other.bn);
            if (!bn) throw std::runtime_error("BN_dup failed");
        }
        return *this;
    }

    BigInt(uint64_t val) : BigInt()
    {
        if (!BN_set_word(bn, static_cast<unsigned long>(val))) throw std::runtime_error("BN_set_word failed");
    }
    BigInt& operator=(BigInt&& other) noexcept {
        if (this != &other) {
            BN_free(bn);
            bn = other.bn;
            other.bn = nullptr;
        }
        return *this;
    }

    ~BigInt() {
        if (bn) BN_free(bn);
    }

    // Преобразование в строку
    std::string toString() const {
        char* str = BN_bn2dec(bn);
        if (!str) throw std::runtime_error("BN_bn2dec failed");
        std::string result(str);
        OPENSSL_free(str);
        return result;
    }

    // Арифметика
    BigInt& operator+=(const BigInt& rhs) {
        BN_add(bn, bn, rhs.bn);
        return *this;
    }
    BigInt& operator=(uint64_t val)
    {
        ensureAlloc();
        if (!BN_set_word(bn, static_cast<unsigned long>(val))) throw std::runtime_error("BN_set_word failed");
        return *this;
    }
    BigInt& operator-=(const BigInt& rhs) {
        BN_sub(bn, bn, rhs.bn);
        return *this;
    }

    BigInt& operator*=(const BigInt& rhs) {
        BN_CTX* ctx = BN_CTX_new();
        BN_mul(bn, bn, rhs.bn, ctx);
        BN_CTX_free(ctx);
        return *this;
    }

    BigInt& operator/=(const BigInt& rhs) {
        BN_CTX* ctx = BN_CTX_new();
        BIGNUM* rem = BN_new();
        BN_div(bn, rem, bn, rhs.bn, ctx);
        BN_free(rem);
        BN_CTX_free(ctx);
        return *this;
    }

    // Сравнения
    bool operator==(const BigInt& rhs) const {
        return BN_cmp(bn, rhs.bn) == 0;
    }

    bool operator!=(const BigInt& rhs) const {
        return !(*this == rhs);
    }

    bool operator<(const BigInt& rhs) const {
        return BN_cmp(bn, rhs.bn) < 0;
    }

    bool operator>(const BigInt& rhs) const {
        return BN_cmp(bn, rhs.bn) > 0;
    }

    bool operator<=(const BigInt& rhs) const {
        return BN_cmp(bn, rhs.bn) <= 0;
    }

    bool operator>=(const BigInt& rhs) const {
        return BN_cmp(bn, rhs.bn) >= 0;
    }

    // Доступ к внутреннему BIGNUM (для низкоуровневых операций)
    BIGNUM* raw() {
        return bn;
    }
    const BIGNUM* raw() const {
        return bn;
    }

    double toDouble() const
    {
        if (BN_is_zero(bn)) return 0.0;
        int numBytes = BN_num_bytes(bn);
        std::vector<unsigned char> buf(numBytes);
        BN_bn2bin(bn, buf.data());
        long double result = 0.0;
        for (int i = 0; i < numBytes; ++i) {
            result = result * 256.0 + buf[i];
        }
        if (BN_is_negative(bn)) result = -result;
        return static_cast<double>(result);
    }
};

// Вспомогательные операторы
//BigInt operator+(BigInt lhs, const BigInt& rhs) { return lhs += rhs; }
//BigInt operator-(BigInt lhs, const BigInt& rhs) { return lhs -= rhs; }
//BigInt operator*(BigInt lhs, const BigInt& rhs) { return lhs *= rhs; }
//BigInt operator/(BigInt lhs, const BigInt& rhs) { return lhs /= rhs; }
inline BigInt operator+(const BigInt& lhs, const BigInt& rhs) {
    BigInt res;
    BN_add(res.raw(), lhs.raw(), rhs.raw());
    return res;
}

inline BigInt operator-(const BigInt& lhs, const BigInt& rhs) {
    BigInt res;
    BN_sub(res.raw(), lhs.raw(), rhs.raw());
    return res;
}

inline BigInt operator*(const BigInt& lhs, const BigInt& rhs) {
    BigInt res;
    BN_CTX* ctx = BN_CTX_new();
    BN_mul(res.raw(), lhs.raw(), rhs.raw(), ctx);
    BN_CTX_free(ctx);
    return res;
}

inline BigInt operator/(const BigInt& lhs, const BigInt& rhs) {
    BigInt res;
    BN_CTX* ctx = BN_CTX_new();
    BIGNUM* rem = BN_new();
    BN_div(res.raw(), rem, lhs.raw(), rhs.raw(), ctx);
    BN_free(rem);
    BN_CTX_free(ctx);
    return res;
}
inline outBuffer& operator<<(outBuffer &o,const BigInt& z)
{
    int len = BN_num_bytes(z.bn);
    unsigned char buf[len];
    BN_bn2bin(z.bn, buf);
    o.put_PN(len);
    o.pack(buf,len);
    return o;
}
inline inBuffer& operator>>(inBuffer &o,BigInt& z)
{
    auto len=o.get_PN();
    unsigned char buf[len];
    o.unpack(buf,len);
    // BIGNUM *
    z.bn=BN_bin2bn(buf, len, z.bn);
    return o;
}
#ifdef __TEST
// Пример использования
int main() {
    BigInt a("12345678901234567890");
    BigInt b("98765432109876543210");

    BigInt c = a + b;
    BigInt d = b - a;
    BigInt e = a * b;
    BigInt f = b / a;

    std::cout << "a+b = " << c.toString() << "\n";
    std::cout << "b-a = " << d.toString() << "\n";
    std::cout << "a*b = " << e.toString() << "\n";
    std::cout << "b/a = " << f.toString() << "\n";

    if (a < b) std::cout << "a < b\n";
}
#endif