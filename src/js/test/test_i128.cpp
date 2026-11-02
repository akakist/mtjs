#include "bc_tools/i128.h"
#include <cassert>
#include <iostream>

int main() {
    // fromString / str
    assert(i128::fromString("123456789012345678901234567890").str() == "123456789012345678901234567890");
    assert(i128::fromString("-42").str() == "-42");
    assert(i128::fromString("+7").str() == "7");
    assert(i128().str() == "0");

    // сравнения
    assert(i128(5) < 10); assert(10 > i128(5));
    assert(i128(5) == 5); assert(5 == i128(5));

    // арифметика
    assert((i128(10) + 5).str() == "15");
    assert((5 + i128(10)).str() == "15");
    assert((i128(10) - 5).str() == "5");
    assert((5 - i128(10)).str() == "-5");
    assert((i128(10) * 3).str() == "30");

    // underflow/overflow
    bool caught = false;
    try { i128 x(5); x -= 10; } catch (std::runtime_error&) { caught = true; }
    assert(caught);

    // toInt64
    assert(i128(42).toInt64() == 42);
    caught = false;
    try { i128::fromString("170141183460469231731687303715884105727").toInt64(); }
    catch (std::runtime_error&) { caught = true; }
    assert(caught);

    // ioBuffer round-trip (put_128/get_128)
    auto rt = [](const i128& a) {
        outBuffer o; o << a;
        inBuffer in(o.asString()->container);
        i128 b; in >> b;
        return b;
    };
    assert(rt(i128(0)) == i128(0));
    assert(rt(i128(42)) == i128(42));
    assert(rt(i128(-1)) == i128(-1));
    assert(rt(i128(INT64_MAX)) == i128(INT64_MAX));
    assert(rt(i128(INT64_MIN)) == i128(INT64_MIN));
    i128 big = i128::fromString("123456789012345678901234567890");
    assert(rt(big) == big);
    i128 neg = i128::fromString("-123456789012345678901234567890");
    assert(rt(neg) == neg);

    // variable-length: малые < 16
    outBuffer o42; o42 << i128(42);
    assert(o42.size() < 16);
    outBuffer o0; o0 << i128(0);
    assert(o0.size() <= 2);

    // GRAN round-trip
    GRAN g(1234567890123456789LL);
    outBuffer og; og << g;
    inBuffer ig(og.asString()->container);
    GRAN g2(0); ig >> g2;
    assert(g == g2);

    std::cout << "i128: all tests passed\n";
    return 0;
}
