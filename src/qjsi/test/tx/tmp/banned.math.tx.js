({
    initial: {},
    transactions: [
        { contract: "banned", method: "badMathRandom", params: {}, expect_error: true },
        { contract: "banned", method: "badMathSqrt",   params: {}, expect_error: true },
        { contract: "banned", method: "badMathMax",    params: {}, expect_error: true }
    ],
    expected: {}
})
