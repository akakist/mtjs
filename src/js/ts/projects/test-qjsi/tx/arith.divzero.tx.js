({
    initial: {},
    transactions: [
        { contract: "arith", method: "div", params: { a: 10, b: 0 }, expect_error: true },
        { contract: "arith", method: "mod", params: { a: 10, b: 0 }, expect_error: true }
    ],
    expected: {}
})
