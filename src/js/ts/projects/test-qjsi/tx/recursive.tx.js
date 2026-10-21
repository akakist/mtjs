({
    initial: {},
    transactions: [
        { contract: "recursive", method: "recurse", params: {}, expect_error: true }
    ],
    expected: {}
})