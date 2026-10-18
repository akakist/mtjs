({
    initial: {},
    transactions: [
        { contract: "banned", method: "badRegexLiteral", params: {}, expect_error: true },
        { contract: "banned", method: "badRegexMatch",   params: {}, expect_error: true },
        { contract: "banned", method: "badRegexReplace", params: {}, expect_error: true },
        { contract: "banned", method: "badRegexSplit",   params: {}, expect_error: true }
    ],
    expected: {}
})
