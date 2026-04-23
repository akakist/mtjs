#pragma once
#include <gmp.h>
#include <string.h>
#include <ioBuffer.h>

struct auto_mpz_t {
    mpz_t value;
    auto_mpz_t() { 
        mpz_init(value); 
        mpz_set_ui(value, 0);
    }
    ~auto_mpz_t() { mpz_clear(value); }

    auto_mpz_t(const auto_mpz_t& other) 
    {
        mpz_init(value);
        mpz_set(value, other.value);
    }
    auto_mpz_t(const uint64_t& other) 
    {
        mpz_init(value);
        mpz_set_ui(value, other);
    }

    auto_mpz_t& operator=(const auto_mpz_t& other) 
    {
        if (this != &other) {
            mpz_set(value, other.value);
        }
        return *this;
    }

    auto_mpz_t& operator=(const uint64_t& other) 
    {
        {
            mpz_set_ui(value, other);
        }
        return *this;
    }

    auto_mpz_t(auto_mpz_t&& other) noexcept 
    {
        mpz_init(value);
        mpz_set(value, other.value);
        mpz_set_ui(other.value, 0); // обнуляем источник
    }

    auto_mpz_t& operator=(auto_mpz_t&& other) noexcept 
    {
        if (this != &other) {
            mpz_set(value, other.value);
            mpz_set_ui(other.value, 0);
        }
        return *this;
    }
    static auto_mpz_t from_decimal(const std::string& s) 
    {
        auto_mpz_t x;
        if (mpz_set_str(x.value, s.c_str(), 10) != 0) {
            throw std::runtime_error("invalid decimal string");
        }
        // mpz_canonicalize(x.value); // нормализуем дробь
        return x;
    }
    void add(const auto_mpz_t& other) 
    {
        mpz_add(value, value, other.value);
    }
    void sub(const auto_mpz_t& other) 
    {
        mpz_sub(value, value, other.value);
    }
    void mul(const auto_mpz_t& other) 
    {
        mpz_mul(value, value, other.value);
    }
    void div(const auto_mpz_t& other) 
    {
        mpz_div(value, value, other.value);
    }
    std::string to_decimal_string() const 
    {
        char* c_str = mpz_get_str(nullptr, 10, value);
        std::string s(c_str);
        void (*freefunc)(void*, size_t);
        mp_get_memory_functions (nullptr, nullptr, &freefunc);
        freefunc(c_str, strlen(c_str) + 1);
        return s;
    }
    auto_mpz_t operator+(const auto_mpz_t& other) const 
    {
        auto_mpz_t result;
        mpz_add(result.value, value, other.value);
        return result;
    }
    auto_mpz_t operator-(const auto_mpz_t& other) const  
    {
        auto_mpz_t result;
        mpz_sub(result.value, value, other.value);
        return result;
    }
    auto_mpz_t operator*(const auto_mpz_t& other) const 
    {
        auto_mpz_t result;
        mpz_mul(result.value, value, other.value);
        return result;
    }
    auto_mpz_t operator/(const auto_mpz_t& other) const 
    {
        auto_mpz_t result;
        mpz_div(result.value, value, other.value);
        return result;
    }
    auto_mpz_t& operator+=(const auto_mpz_t& other) 
    {
        mpz_add(value, value, other.value);
        return *this;
    }
    auto_mpz_t& operator-=(const auto_mpz_t& other) 
    {
        mpz_sub(value, value, other.value);
        return *this;
    }
    auto_mpz_t& operator*=(const auto_mpz_t& other) 
    {
        mpz_mul(value, value, other.value);
        return *this;
    }
    auto_mpz_t& operator/=(const auto_mpz_t& other)
    {
        mpz_div(value, value, other.value);
        return *this;
    }   
    int operator==(const auto_mpz_t& other) const 
    {
        return mpz_cmp(value, other.value)==0;
    }
    int operator!=(const auto_mpz_t& other) const 
    {
        return !mpz_cmp(value, other.value)!=0;
    }
    int operator<(const auto_mpz_t& other) const 
    {
        return mpz_cmp(value, other.value) < 0;
    }
    int operator<=(const auto_mpz_t& other) const 
    {
        return mpz_cmp(value, other.value) <= 0;
    }
    int operator>(const auto_mpz_t& other) const 
    {
        return mpz_cmp(value, other.value) > 0;
    }
    int operator>=(const auto_mpz_t& other) const 
    {
        return mpz_cmp(value, other.value) >= 0;
    }   

};

inline outBuffer& operator<<(outBuffer &o,const auto_mpz_t& z)
{
    o<<z.to_decimal_string();
    return o;
}
inline inBuffer& operator>>(inBuffer &o,auto_mpz_t& z)
{
    z=auto_mpz_t::from_decimal(o.get_PSTR());
    return o;
}
inline auto_mpz_t getRandom(const auto_mpz_t& max)
{
    gmp_randstate_t state;
    gmp_randinit_default(state);
    auto_mpz_t r;
    mpz_urandomm(r.value, state, max.value);
    gmp_randclear(state);
    return r;
}