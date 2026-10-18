({
    initial: {},
    transactions: [
        { contract: "arith", method: "add", params: { a: 1, b: 2 }, expect_result: 3 },
        { contract: "arith", method: "sub", params: { a: 5, b: 3 }, expect_result: 2 },
        { contract: "arith", method: "mul", params: { a: 4, b: 6 }, expect_result: 24 },
        { contract: "arith", method: "div", params: { a: 10, b: 3 }, expect_result: 3 },
        { contract: "arith", method: "mod", params: { a: 10, b: 3 }, expect_result: 1 },
        { contract: "arith", method: "neg", params: { a: 7 }, expect_result: -7 },
        { contract: "arith", method: "inc", params: { a: 41 }, expect_result: 42 },
        { contract: "arith", method: "dec", params: { a: 43 }, expect_result: 42 }
    ],
    expected: {}
})
