({
    initial: {},
    transactions: [
        { contract: "transfer", method: "transfer",
          params: { from: "g1", to: "g2", amount: 100 },
          expect_error: true }
    ],
    expected: {}
})
