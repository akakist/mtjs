({
    time: 1000,
    initial: {
        "transfer/g1": { balance: 1000 },
        "transfer/g2": { balance: 500 }
    },
    transactions: [
        { msg: { sender: "alice", blockTimestamp: 1000, chainId: 1 },
          commands: [
              { contract: "transfer", method: "transfer",
                params: { from: "g1", to: "g2", amount: 100 },
                expect_events: [
                    { contract: "transfer", name: "Transfer",
                      value: { from: "g1", to: "g2", amount: 100, sender: "alice" } }
                ] }
          ] }
    ],
    expected_emits: [
        { contract: "transfer", name: "Transfer",
          value: { from: "g1", to: "g2", amount: 100, sender: "alice" } }
    ],
    expected: {
        "transfer/g1": { balance: 900 },
        "transfer/g2": { balance: 600 }
    }
})