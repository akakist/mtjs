({
    initial: {},
    transactions: [
        { contract: "banned", method: "badEval",                 params: {}, expect_error: true },
        { contract: "banned", method: "badFunction",             params: {}, expect_error: true },
        { contract: "banned", method: "badConstructorConstructor", params: {}, expect_error: true }
    ],
    expected: {}
})
