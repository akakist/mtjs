({
    initial: {
        "transfer/g1": { balance: 1000 }
    },
    transactions: [
        { contract: "observer", method: "peek",
          params: { address: "g1" },
          expect_result: { balance: 1000 } }
    ],
    expected: {
        "transfer/g1": { balance: 1000 }
    }
})
