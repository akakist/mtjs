({
    initial: {},
    transactions: [
        { contract: "banned", method: "badDate",    params: {}, expect_error: true },
        { contract: "banned", method: "badDateNow", params: {}, expect_error: true }
    ],
    expected: {}
})
