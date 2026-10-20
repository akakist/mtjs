({
    initial: {},
    transactions: [
        { contract: "banned", method: "badInt8Array",   params: {}, expect_error: true },
        { contract: "banned", method: "badArrayBuffer", params: {}, expect_error: true },
        { contract: "banned", method: "badDataView",    params: {}, expect_error: true }
    ],
    expected: {}
})
