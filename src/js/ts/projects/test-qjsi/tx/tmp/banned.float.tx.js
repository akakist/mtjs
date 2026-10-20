({
    initial: {},
    transactions: [
        { contract: "banned_float", method: "badFloatLiteral",  params: {}, expect_error: true },
        { contract: "banned_float", method: "badFloatExponent", params: {}, expect_error: true },
        { contract: "banned_float", method: "badNaN",           params: {}, expect_error: true },
        { contract: "banned_float", method: "badInfinity",      params: {}, expect_error: true },
        { contract: "banned_float", method: "badParseFloat",    params: {}, expect_error: true }
    ],
    expected: {}
})
