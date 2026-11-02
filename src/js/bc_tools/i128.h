// bc_tools/i128.h
#pragma once
#include <cstdint>
#include <string>
#include <stdexcept>
#include "ioBuffer.h"   // outBuffer, inBuffer, put_PN, get_PN

struct i128 {
    __int128 v;

    explicit i128(int64_t x) : v((__int128)x) {}
    i128() : v(0) {}

    static i128 fromString(const std::string& s) {
        if (s.empty()) throw std::runtime_error("i128::fromString: empty");
        size_t i = 0; bool neg = false;
        if (s[0] == '-' || s[0] == '+') { neg = (s[0] == '-'); i = 1; }
        if (i == s.size()) throw std::runtime_error("i128::fromString: no digits");
        unsigned __int128 r = 0;
        const unsigned __int128 max_pos = ((unsigned __int128)1 << 127) - 1;
        for (; i < s.size(); ++i) {
            char c = s[i];
            if (c < '0' || c > '9') throw std::runtime_error("i128::fromString: bad char");
            unsigned d = (unsigned)(c - '0');
            if (r > (max_pos - d) / 10)
                throw std::runtime_error("i128::fromString: overflow");
            r = r * 10 + d;
        }
        i128 out;
        out.v = neg ? -(__int128)r : (__int128)r;
        return out;
    }

    std::string str() const {
        if (v == 0) return "0";
        __int128 x = v; bool neg = x < 0;
        if (neg) x = -x;
        char buf[64]; int n = 0;
        while (x > 0) { buf[n++] = '0' + (int)(x % 10); x /= 10; }
        std::string s;
        if (neg) s.push_back('-');
        while (n > 0) s.push_back(buf[--n]);
        return s;
    }

    // сравнения (i128 / int64)
    bool operator<(const i128& o) const  { return v < o.v; }
    bool operator<=(const i128& o) const { return v <= o.v; }
    bool operator>(const i128& o) const  { return v > o.v; }
    bool operator>=(const i128& o) const { return v >= o.v; }
    bool operator==(const i128& o) const { return v == o.v; }
    bool operator!=(const i128& o) const { return v != o.v; }
    bool operator<(int64_t o) const  { return v < (__int128)o; }
    bool operator<=(int64_t o) const { return v <= (__int128)o; }
    bool operator>(int64_t o) const  { return v > (__int128)o; }
    bool operator>=(int64_t o) const { return v >= (__int128)o; }
    bool operator==(int64_t o) const { return v == (__int128)o; }
    bool operator!=(int64_t o) const { return v != (__int128)o; }

    // арифметика
    i128& operator+=(const i128& o) {
        __int128 r;
        if (__builtin_add_overflow(v, o.v, &r)) throw std::runtime_error("i128: add overflow");
        v = r; return *this;
    }
    i128& operator-=(const i128& o) {
        __int128 r;
        if (__builtin_sub_overflow(v, o.v, &r)) throw std::runtime_error("i128: sub overflow");
        v = r; return *this;
    }
    i128& operator*=(const i128& o) {
        __int128 r;
        if (__builtin_mul_overflow(v, o.v, &r)) throw std::runtime_error("i128: mul overflow");
        v = r; return *this;
    }
    i128& operator+=(int64_t o) { return *this += i128(o); }
    i128& operator-=(int64_t o) { return *this -= i128(o); }
    i128& operator*=(int64_t o) { return *this *= i128(o); }

    int64_t toInt64() const {
        if (v < (__int128)INT64_MIN || v > (__int128)INT64_MAX)
            throw std::runtime_error("i128::toInt64: out of range");
        return (int64_t)v;
    }
};

// бинарные i128 (через копию + +=)
inline i128 operator+(i128 a, const i128& b) { a += b; return a; }
inline i128 operator-(i128 a, const i128& b) { a -= b; return a; }
inline i128 operator*(i128 a, const i128& b) { a *= b; return a; }

// i128 <-> int64
inline i128 operator+(i128 a, int64_t b) { a += b; return a; }
inline i128 operator+(int64_t a, i128 b) { b += a; return b; }
inline i128 operator-(i128 a, int64_t b) { a -= b; return a; }
inline i128 operator-(int64_t a, i128 b) { b = i128(a) - b; return b; }
inline i128 operator*(i128 a, int64_t b) { a *= b; return a; }
inline i128 operator*(int64_t a, i128 b) { b *= a; return b; }
inline bool operator<(int64_t a, const i128& b)  { return (__int128)a < b.v; }
inline bool operator<=(int64_t a, const i128& b) { return (__int128)a <= b.v; }
inline bool operator>(int64_t a, const i128& b)  { return (__int128)a > b.v; }
inline bool operator>=(int64_t a, const i128& b) { return (__int128)a >= b.v; }
inline bool operator==(int64_t a, const i128& b) { return (__int128)a == b.v; }
inline bool operator!=(int64_t a, const i128& b) { return (__int128)a != b.v; }

// ---- ioBuffer: inline-хелперы (без правки ioBuffer.h) ----
inline void i128_put(outBuffer& o, __int128 v) {
    unsigned __int128 u = (unsigned __int128)v;
    unsigned __int128 z = (u << 1) ^ (unsigned __int128)(v >> 127);  // zigzag
    o.put_PN((uint64_t)z);
    o.put_PN((uint64_t)(z >> 64));
}
inline __int128 i128_get(inBuffer& o) {
    uint64_t lo = o.get_PN();
    uint64_t hi = o.get_PN();
    unsigned __int128 z = ((unsigned __int128)hi << 64) | lo;
    return (__int128)(z >> 1) ^ -(__int128)(z & 1);   // обратный zigzag
}

inline outBuffer& operator<<(outBuffer& o, const i128& z) { i128_put(o, z.v); return o; }
inline inBuffer& operator>>(inBuffer& o, i128& z) { z.v = i128_get(o); return o; }

// ---- GRAN ----
struct GRAN {
    i128 v;
    explicit GRAN(int64_t x) : v(x) {}
    GRAN() : v(0) {}
    static GRAN fromString(const std::string& s) { GRAN g; g.v = i128::fromString(s); return g; }
    std::string str() const { return v.str(); }
    GRAN& operator+=(const GRAN& o) { v += o.v; return *this; }
    GRAN& operator-=(const GRAN& o) { v -= o.v; return *this; }
    bool operator<(const GRAN& o) const { return v < o.v; }
    bool operator==(const GRAN& o) const { return v == o.v; }
};
inline outBuffer& operator<<(outBuffer& o, const GRAN& z) { return o << z.v; }
inline inBuffer& operator>>(inBuffer& o, GRAN& z) { return o >> z.v; }