({
    initial: {},
    transactions: [
        { contract: "observer", method: "callBad",
          params: {}, expect_error: true }
    ],
    expected: {}
})
