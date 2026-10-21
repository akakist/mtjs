({
    initial: {},
    transactions: [
        { contract: "int64edge", method: "negOfInt64Min",  params: {}, expect_error: true },
        { contract: "int64edge", method: "incOfInt64Max",  params: {}, expect_error: true },
        { contract: "int64edge", method: "decOfInt64Min",  params: {}, expect_error: true },
        { contract: "int64edge", method: "negOfInt64Max",  params: {}, expect_result: -9223372036854775807 }
    ],
    expected: {}
})