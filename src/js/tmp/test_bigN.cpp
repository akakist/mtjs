#include "bigN.h"
#include <iostream>
#include <cassert>
#include <cstdint>

// тесты BigN — используют только целочисленное деление div()
static void run_bigN_more_tests()
{
    // большие десятичные числа (через парсер строк)
    auto pa = BigN::from_decimal_string("123456789012345678901234567890");
    auto pb = BigN::from_decimal_string("98765432109876543210");
    BigN A = pa;
    BigN B = pb;

    // A + B, затем (A+B) - B == A
    BigN C = A + B;
    BigN CminusB = C - B;
    assert(!(CminusB.isInf()));
    assert(CminusB.to_string() == A.to_string());
    std::cout << "addition/subtraction identity OK\n";

    // умножение на small uint и обратная операция: (A * 3) / 3 == A
    BigN three = BigN::from_uint64(3);
    BigN A3 = A * three;
    BigN A3_div3 = A3.div(three);
    assert(A3_div3.to_string() == A.to_string());
    std::cout << "mul by 3 and div by 3 identity OK\n";

    // проверка известного произведения (123456789 * 987654321 = 121932631112635269)
    auto p1 = BigN::from_decimal_string("123456789");
    auto p2 = BigN::from_decimal_string("987654321");
    BigN prod = p1 * p2;
    assert(prod.to_string() == std::string("121932631112635269"));
    std::cout << "known product check OK\n";

    // деление точное: prod / p1 == p2
    BigN quotient = prod.div(p1);
    assert(quotient.to_string() == p2.to_string());
    std::cout << "division exact quotient OK\n";

    // --- divmod tests ---
    {
        // product / p1 => (p2, 0)
        auto dm = prod.divmod(p1);
        assert(dm.first.to_string() == p2.to_string());
        assert(dm.second.isZero());
        std::cout << "divmod: product / p1 => quotient OK, remainder == 0\n";
    }

    {
        // small integer example: 100 / 3 => (33,1)
        BigN a = BigN::from_uint64(100);
        BigN b = BigN::from_uint64(3);
        auto pr = a.divmod(b);
        assert(pr.first.to_string() == "33");
        assert(pr.second.to_string() == "1");
        assert(pr.second < b); // remainder < divisor
        // reconstruction: q*b + r == a
        BigN recon = pr.first * b;
        recon += pr.second;
        assert(recon.to_string() == a.to_string());
        std::cout << "divmod 100/3 -> (33,1) and reconstruction OK\n";
    }

    {
        // division by 1: remainder == 0, quotient == original
        BigN one = BigN::from_uint64(1);
        auto pr = A.divmod(one);
        assert(pr.second.isZero());
        assert(pr.first.to_string() == A.to_string());
        std::cout << "divmod by 1 OK\n";
    }

    {
        // division by zero: quotient marked inf, remainder == original
        BigN zero;
        auto pr = A.divmod(zero);
        assert(pr.first.isInf());
        assert(pr.second.to_string() == A.to_string());
        std::cout << "divmod by zero -> quotient inf, remainder preserved OK\n";
    }
    // --- end divmod tests ---

    // проверка сравнения
    assert(A < C);
    assert(!(C < A));
    std::cout << "comparison OK\n";

    // тест битовых операций: set/get бит 100
    BigN X;
    X.setBit(100, 1);
    assert(X.getBit(100) == 1);
    // сбросить бит
    X.setBit(100, 0);
    assert(X.getBit(100) == 0);
    std::cout << "bit set/get OK\n";

    // div_with_precision small test: 1/2 with frac_bits=8 => 128
    // BigN one = BigN::from_uint64(1);
    // BigN two = BigN::from_uint64(2);
    // BigN q = one.div_with_precision(two, 8);
    // assert(q.to_string() == "128");
    // std::cout << "div_with_precision (1/2, 8 bits) OK\n";

    // деление на 0 -> inf
    BigN zero;
    BigN r = A.div(zero);
    assert(r.isInf());
    std::cout << "division by zero -> inf OK\n";

    // стресс: повторяем умножение/сложение для роста числа (непредельный, но большой)
    BigN big = BigN::from_uint64(1);
    for(int i=0;i<40;i++){
        big = big * BigN::from_uint64(3); // умножаем 40 раз
    }
    // простая проверка: big > 0
    assert(!big.isZero());
    std::cout << "growth multiplication OK (40x *3)\n";


    {
    BigN val=BigN::from_uint64(1);
    val.shiftBits(64);
    BigN v1=BigN::from_uint64(2);
    val=val+v1;
    printf  ("gas=%s\n",val.to_hex().c_str());
    printf  ("gas_canonical=%s\n",val.to_hex_canonical().c_str());

    }

    BigN val = BigN::from_uint64(1);
    printf("orig: debug=%s  to_string=%s\n", val.debug().c_str(), val.to_string().c_str());
    val.shiftBits(64);
    printf("after shift: debug=%s  to_string=%s\n", val.debug().c_str(), val.to_string().c_str());
    BigN v1 = BigN::from_uint64(2);
    val = val + v1;
    printf("after add 2: debug=%s  to_string=%s\n", val.debug().c_str(), val.to_string().c_str());

    // Проверка задания 2^64 и сложения с 2
    {
        BigN val;
        val.setBit(64, 1);               // задаём 2^64 явно
        BigN v1 = BigN::from_uint64(2);  // 2
        val = val + v1;
        printf("setBit64 + 2 -> debug=%s  to_string=%s\n", val.debug().c_str(), val.to_string().c_str());
        // ожидаем 18446744073709551618
        assert(val.to_string() == std::string("18446744073709551618"));
        std::cout << "setBit(64) + 2 OK\n";
    }

    std::cout << "all additional bigN tests passed\n";
}

static void run_bigN_tests()
{
    // to_string test
    {
        BigN a = BigN::from_uint64(12345);
        std::string s = a.to_string();
        assert(s == "12345");
        printf("to_string(12345) = %s\n", s.c_str());
    }

    // integer division test: 100 / 3 == 33
    {
        BigN a = BigN::from_uint64(100);
        BigN b = BigN::from_uint64(3);
        BigN q = a.div(b);
        printf("100 / 3 = %s\n", q.to_string().c_str());
        assert(q.to_string() == "33");
    }

    // from decimal string (no floats)
    // {
    //     auto pr = BigN::from_decimal_string("0.44345");
    //     BigN x = pr.first;
    //     int decimals = pr.second;
    //     printf("from_decimal debug: %s\n", x.debug().c_str());
    //     std::string s = x.to_string(decimals);
    //     printf("from_decimal(\"0.44345\") -> %s (decimals=%d)\n", s.c_str(), decimals);
    //     assert(s == "0.44345");
    // }

    // division by zero -> isInf
    {
        BigN a = BigN::from_uint64(42);
        BigN z; // zero
        BigN r = a.div(z);
        assert(r.isInf());
        printf("42 / 0 -> inf\n");
    }

    // trim + setBit basic sanity
    {
        BigN x;
        x.setBit(16, 1); // set bit 16
        x.trim();
        BigN expected = BigN::from_uint64(1ull << 16);
        assert(x.to_string() == expected.to_string());
        printf("setBit/trim sanity passed\n");
    }

    std::cout << "bigN tests passed\n";
}

int main()
{
    run_bigN_tests();
    run_bigN_more_tests();    

    auto f=factorial_seq(2000);
    std::cout<<"2000! size bytes="<<f.to_string()<<std::endl;
    return 0;
}