({
    initial: {},
    transactions: [
        { contract: "banned", method: "badMap",     params: {}, expect_error: true },
        { contract: "banned", method: "badSet",     params: {}, expect_error: true },
        { contract: "banned", method: "badWeakMap", params: {}, expect_error: true },
        { contract: "banned", method: "badWeakSet", params: {}, expect_error: true }
    ],
    expected: {}
})
