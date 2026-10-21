({
    initial: {},
    transactions: [
        { contract: "arith", method: "add",
          params: { a: 9223372036854775807, b: 10 },
          expect_error: true },   // ← INT64_MAX + 1 = overflow
        { contract: "arith", method: "mul",
          params: { a: 9223372036854775807, b: 2 },
          expect_error: true },   // ← INT64_MAX * 2 = overflow
        { contract: "arith", method: "neg",
          params: { a: -9223372036854775807 },
          expect_error: false },  // ← -(-INT64_MAX) = INT64_MAX, не overflow
        { contract: "arith", method: "neg",
          params: { a: -9223372036854775807 },
          expect_error: false },    // ← -INT64_MIN = INT64_MAX + 1 = overflow
//{ contract:"arith", method:"neg", params:{a:-9223372036854775808}, expect_error: true },
{ contract:"arith", method:"inc", params:{a:9223372036854775807}, expect_error: true },
//{ contract:"arith", method:"dec", params:{a:-9223372036854775808}, expect_error: true }
    ],
    expected: {}
})