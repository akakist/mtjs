({
    initial: {},
    transactions: [
        { contract: "banned", method: "badPromise", params: {}, expect_error: true },
        { contract: "banned", method: "badAsync",   params: {}, expect_error: true },
        { contract: "banned", method: "badAwait",   params: {}, expect_error: true }
    ],
    expected: {}
})
