({
    initial: {},
    transactions: [
        { contract: "banned", method: "badProxy",          params: {}, expect_error: true },
        { contract: "banned", method: "badProxyRevocable", params: {}, expect_error: true },
        { contract: "banned", method: "badReflect",        params: {}, expect_error: true }
    ],
    expected: {}
})
