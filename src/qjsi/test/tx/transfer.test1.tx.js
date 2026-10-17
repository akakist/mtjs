({
    initial: {
        "transfer/g1": { balance: 1000 },
        "transfer/g2": { balance: 500 }
    },
    transactions: [
        { contract: "transfer", method: "transfer",
          params: { from: "g1", to: "g2", amount: 100 } }
    ],
    expected: {
        "transfer/g1": { balance: 900 },
        "transfer/g2": { balance: 600 }
    }
})