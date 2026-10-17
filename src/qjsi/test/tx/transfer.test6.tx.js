({
    initial: {
        "transfer/g1": { balance: 777 }
    },
    transactions: [
        { contract: "observer", method: "peekBalance",
          params: { address: "g1" },
          expect_result: 777 }
    ],
    expected: {
        "transfer/g1": { balance: 777 }
    }
})
