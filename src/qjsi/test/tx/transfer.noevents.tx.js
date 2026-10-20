({
    time: 1000,
    initial: {
        "transfer/g1": { balance: 100 },
        "transfer/g2": { balance: 500 }
    },
    transactions: [
        { msg: { sender: "alice", blockTimestamp: 1000 },
          commands: [
              { contract: "transfer", method: "transfer",
                params: { from: "g1", to: "g2", amount: 1000 },
                expect_error: true }
          ] }
    ],
    expected_emits: [],
    expected: {
        "transfer/g1": { balance: 100 },
        "transfer/g2": { balance: 500 }
    }
})