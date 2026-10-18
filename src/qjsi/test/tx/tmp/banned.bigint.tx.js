({
    initial: {},
    transactions: [
        { contract: "banned", method: "badBigInt",        params: {}, expect_error: true },
        { contract: "banned", method: "badBigIntLiteral", params: {}, expect_error: true }
    ],
    expected: {}
})
