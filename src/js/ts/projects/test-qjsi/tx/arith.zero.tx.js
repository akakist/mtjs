({
    initial: {},
    transactions: [
        { contract: "arith", method: "add", params: { a: 0, b: 0 }, expect_result: 0 },
        { contract: "arith", method: "sub", params: { a: 0, b: 0 }, expect_result: 0 },
        { contract: "arith", method: "mul", params: { a: 0, b: 12345 }, expect_result: 0 },
        { contract: "arith", method: "neg", params: { a: 0 }, expect_result: 0 },
        { contract: "arith", method: "inc", params: { a: -1 }, expect_result: 0 },
        { contract: "arith", method: "dec", params: { a: 1 }, expect_result: 0 }
    ],
    expected: {}
})
