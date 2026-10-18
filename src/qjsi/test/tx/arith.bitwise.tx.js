({
    initial: {},
    transactions: [
        { contract: "arith", method: "bitand", params: { a: 0b1100, b: 0b1010 }, expect_result: 0b1000 },
        { contract: "arith", method: "bitor",  params: { a: 0b1100, b: 0b1010 }, expect_result: 0b1110 },
        { contract: "arith", method: "bitxor", params: { a: 0b1100, b: 0b1010 }, expect_result: 0b0110 },
        { contract: "arith", method: "shl",    params: { a: 1, b: 4 }, expect_result: 16 },
        { contract: "arith", method: "shr",    params: { a: 16, b: 4 }, expect_result: 1 }
    ],
    expected: {}
})
