({
    initial: {},
    transactions: [
        { contract: "arith", method: "add", params: { a: 9007199254740993, b: 1 }, expect_result: 9007199254740994 },
        { contract: "arith", method: "mul", params: { a: 1000000000, b: 1000000000 }, expect_result: 1000000000000000000 },
        { contract: "arith", method: "sub", params: { a: 9223372036854775807, b: 1 }, expect_result: 9223372036854775806 }
    ],
    expected: {}
})
