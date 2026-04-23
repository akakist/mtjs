#pragma once
#include <vector>
#include <deque>
#include <cstdint>
#include <algorithm>
#include <string>
#include <utility>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <inttypes.h>

class BigN
{
    std::deque < uint8_t > data;
    int flags = 0; // битовые флаги состояния
    enum FlagBits {
        ISINF = 1 << 0,
    };

    public:
    int getMinPos() const { return 0; } // теперь позиция младшего бита всегда 0

    // Создать BigN из uint64_t
    static BigN from_uint64(uint64_t v)
    {
        BigN x;
        for(int i = 0; i < 64; ++i) {
            if((v >> i) & 1ull) x.setBit(i, 1);
        }
        x.trim();
        return x;
    }

    static BigN from_decimal_string(const std::string &s)
    {
        std::string digits;
        bool seen_dot = false;
        int frac_len = 0;
        for(char c : s) {
            if(c == '.' && !seen_dot) {
                seen_dot = true;
                continue;
            }
            if(c >= '0' && c <= '9') {
                digits.push_back(c);
                if(seen_dot) ++frac_len;
            }
            // ignore other chars (sign handling could be added if needed)
        }
        // remove leading zeros (but keep at least one digit)
        size_t p = 0;
        while(p + 1 < digits.size() && digits[p] == '0') ++p;
        if(p) digits.erase(0, p);

        BigN res;
        BigN ten = BigN::from_uint64(10);
        for(char c : digits) {
            int d = c - '0';
            res = res * ten;
            if(d) {
                BigN add = BigN::from_uint64((uint64_t)d);
                res += add;
            }
        }
        res.trim();
        return { res };
    }

    // Убирает ведущие нулевые байты (most-significant)
    void trim()
    {
        while(!data.empty() && data.back() == 0) data.pop_back();
    }

    // Проверка на ноль
    bool isZero() const
    {
        if(data.empty()) return true;
        for(auto b : data) if(b) return false;
        return true;
    }

    // Отладочная информация: содержимое data (little-endian)
    std::string debug() const
    {
        std::ostringstream oss;
        oss << "size=" << data.size() << " bytes=[";
        oss << std::hex;
        for(size_t i = 0; i < data.size(); ++i) {
            if(i) oss << ",";
            oss << std::setw(2) << std::setfill('0') << (int)data[i];
        }
        oss << "]" << std::dec;
        return oss.str();
    }

    // to_hex возвращает канонический числовой hex (big-endian) — эквивалент to_hex_canonical.
    std::string to_hex() const
    {
        return to_hex_canonical();
    }

    // Canonical hex: вычисляет hex по числовому значению (делением на 256).
    // Надежно показывает действительную числовую hex-репрезентацию (MSB..LSB).
    std::string to_hex_canonical() const
    {
        if(isZero()) {
            return std::string("0x0");
        }
        BigN tmp = *this;
        tmp.trim();
        std::vector<uint8_t> bytes;
        while(!tmp.isZero()) {
            auto pr = tmp.divmod_small(256); // returns (quotient, remainder)
            bytes.push_back((uint8_t)pr.second);
            tmp = pr.first;
        }
        std::ostringstream oss;
        oss << "0x";
        oss << std::hex << std::setfill('0');
        for(int i = (int)bytes.size() - 1; i >= 0; --i) {
            oss << std::setw(2) << (int)bytes[i];
        }
        oss << std::dec;
        return oss.str();
    }

    // Деление на маленькое целое: возвращает (quotient, remainder).
    // Использует бинарное "в столбик" деление по битам.
    std::pair<BigN,int64_t> divmod_small(int64_t divisor) const
    {
        BigN q;
        if(divisor <= 0) return {q, 0};
        // zero
        int hi = (int)data.size()*8 - 1;
        if(hi < 0) return {q, 0};
        int64_t rem = 0;
        for(int i = hi; i >= 0; --i)
        {
            rem = (rem << 1) | (int64_t)getBit(i);
            if(rem >= divisor)
            {
                q.setBit(i, 1);
                rem -= divisor;
            }
        }
        q.trim();
        return { q, rem };
    }

    // Конвертация в десятичную строку. Для скорости используем базу 1e9.
    // decimals — число знаков после запятой (если число представляли как целое с scale).
    std::string to_string(int decimals = 0) const
    {
        if(isInf()) return "inf";
        if(isZero()) {
            if(decimals > 0) {
                std::string s = "0.";
                s.append(decimals, '0');
                return s;
            }
            return "0";
        }

        // клонируем временно число
        BigN tmp = *this;
        tmp.trim();

        const int BASE = 1000000000; // 1e9
        std::vector<uint32_t> parts;
        while(!tmp.isZero())
        {
            auto pr = tmp.divmod_small(BASE);
            int64_t rem = pr.second;
            parts.push_back((uint32_t)rem);
            tmp = pr.first;
        }

        // сборка строки из частей
        std::ostringstream oss;
        if(parts.empty()) oss << '0';
        else {
            // старшая часть без лидирующих нулей
            oss << parts.back();
            for(int i = (int)parts.size() - 2; i >= 0; --i) {
                oss << std::setw(9) << std::setfill('0') << parts[i];
            }
        }
        std::string s = oss.str();

        if(decimals > 0) {
            if((int)s.size() <= decimals) {
                std::string pad(decimals + 1 - s.size(), '0');
                s = pad + s;
            }
            int pos = (int)s.size() - decimals;
            s.insert(pos, 1, '.');
            // trim trailing zeros in fractional part
            while(!s.empty() && s.back() == '0') s.pop_back();
            if(!s.empty() && s.back() == '.') s.push_back('0');
        }
        return s;
    }

    // Flags accessors
    void setInf(bool v = true) { if(v) flags |= ISINF; else flags &= ~ISINF; }
    bool isInf() const { return (flags & ISINF) != 0; }
    void clearInf() { flags &= ~ISINF; }

    // Division precision members removed — используйте div_with_precision(...)

    int getBit(int N) const
    {
        if(N < 0) return 0;
        int nbyte = N / 8;
        if(nbyte >= (int)data.size()) return 0;
        int bitpos = N % 8;
        // return (data[nbyte] >> (7 - bitpos)) & 1;
        return (data[nbyte] >> (bitpos)) & 1;
    }
    void setBit(int N, int v)
    {   
        if(N < 0) return;
        int nbyte = N / 8;
        if(nbyte >= (int)data.size()) data.resize(nbyte + 1, 0);
        int bitpos = N % 8;
        // if(v) data[nbyte] |= (1 << (7 - bitpos));
        // else data[nbyte] &= ~(1 << (7 - bitpos));
        if(v) data[nbyte] |= (1 << (bitpos));
        else data[nbyte] &= ~(1 << (bitpos));
    }
    BigN & operator+(const BigN &b)
    {
        int maxbits = std::max((int)data.size()*8, (int)b.data.size()*8);
        BigN res;
        res.data.resize((maxbits + 7) / 8, 0);
        int carry = 0;
        for(int i = 0; i < maxbits || carry; ++i) {
            int bit1 = getBit(i);
            int bit2 = b.getBit(i);
            int sum = bit1 + bit2 + carry;
            res.setBit(i, sum & 1);
            carry = (sum >> 1) & 1;
        }
        res.trim();
        *this = res;
        return *this;
    }  
    BigN & operator+=(const BigN &b)
    {
        *this = *this + b;
        return *this;
    }
    BigN& operator=(const BigN &b)
    {
        data=b.data;
        // exponent removed
        flags=b.flags;
        // div_precision/dec_scale removed
        return *this;
    }
    void shiftBits(int n)
    {
        if (n == 0) return;
        BigN src = *this;
        this->data.clear();
        if(src.data.empty()) return;
        int src_bits = (int)src.data.size() * 8;
        for(int bit = 0; bit < src_bits; ++bit) {
            if(src.getBit(bit)) this->setBit(bit + n, 1);
        }
        this->trim();
    }
    BigN operator *(const BigN &b) const
    {
        BigN res;
        int bits = (int)data.size()*8;
        for(int i=0;i<bits;i++)
        {
            if(getBit(i))
            {
                BigN t = b;
                t.shiftBits(i);
                res += t;
            }
        }
        return res;
    }
    BigN & operator*=(const BigN &b)
    {
        *this = *this * b;
        return *this;
    }
    BigN & operator-(const BigN &b)
    {
        BigN res;
        int maxbits = std::max((int)data.size()*8, (int)b.data.size()*8);
        res.data.resize((maxbits + 7) / 8, 0);
        int borrow = 0;
        for(int i = 0; i < maxbits; ++i) {
            int bit1 = getBit(i);
            int bit2 = b.getBit(i);
            int sub = bit1 - bit2 - borrow;
            if(sub < 0) { sub += 2; borrow = 1; } else borrow = 0;
            res.setBit(i, sub & 1);
        }
        res.trim();
        *this = res;
        return *this;
    }
    BigN & operator-=(const BigN &b)
    {
        *this = *this - b;
        return *this;
    }
    
    // Деление реализовано только через div_with_precision(...)
    // operator/ и operator/= удалены намеренно, чтобы избежать неоднозначностей с точностью.
    bool operator < (const BigN& b) const
    {
        int this_bits = (int)data.size()*8;
        int b_bits = (int)b.data.size()*8;
        if(this_bits != b_bits) return this_bits < b_bits;
        for(int i = this_bits - 1; i >= 0; --i) {
            int bit1 = getBit(i);
            int bit2 = b.getBit(i);
            if(bit1 != bit2) return bit1 < bit2;
        }
        return false;
    }

    // divmod: возвращает (quotient, remainder) — целочисленное деление.
    std::pair<BigN, BigN> divmod(const BigN& b) const
    {
        BigN q;
        BigN r = *this;
        if(b.data.empty()) {
            q.setInf(true);
            return { q, r };
        }
        r.trim();
        BigN dv = b;
        dv.trim();
        if(r.isZero()) {
            q.trim();
            r.trim();
            return { q, r };
        }
        if(r < dv) {
            q.trim();
            r.trim();
            return { q, r };
        }

        int bitlen_a = (int)r.data.size()*8;
        int bitlen_b = (int)dv.data.size()*8;
        int shift = bitlen_a - bitlen_b;

        // shifted divisor
        BigN sd = dv;
        sd.shiftBits(shift);

        for(int s = shift; s >= 0; --s) {
            if(!(r < sd)) {
                r = r - sd;
                q.setBit(s, 1);
            }
            sd.shiftBits(-1);
        }
        q.trim();
        r.trim();
        return { q, r };
    }

    // div() остаётся удобным вызовом, возвращает только частное
    BigN div(const BigN& b) const
    {
        return divmod(b).first;
    }

};
inline BigN factorial_seq(int n)
{
    BigN res = BigN::from_uint64(1);
    for(int i = 2; i <= n; ++i) {
        printf("i %d\r", i);
        res *= BigN::from_uint64((uint64_t)i);
    }
    return res;
}