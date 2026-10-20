({
    initial: {},
    transactions: [
        { contract: "arith", method: "lt",  params: { a: 1, b: 2 }, expect_result: true },
        { contract: "arith", method: "lt",  params: { a: 2, b: 2 }, expect_result: false },
        { contract: "arith", method: "le",  params: { a: 2, b: 2 }, expect_result: true },
        { contract: "arith", method: "gt",  params: { a: 3, b: 2 }, expect_result: true },
        { contract: "arith", method: "ge",  params: { a: 2, b: 2 }, expect_result: true },
        { contract: "arith", method: "eq",  params: { a: 2, b: 2 }, expect_result: true },
        { contract: "arith", method: "eq",  params: { a: 2, b: 3 }, expect_result: false },
        { contract: "arith", method: "neq", params: { a: 2, b: 3 }, expect_result: true },
        { contract: "arith", method: "lt",  params: { a: -1, b: 0 }, expect_result: true },
        { contract: "arith", method: "gt",  params: { a: -1, b: -2 }, expect_result: true }
    ],
    expected: {}
})
