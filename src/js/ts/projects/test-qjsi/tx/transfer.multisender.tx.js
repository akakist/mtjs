({
    time: 1000,
    initial: {
        "transfer/g1": { balance: 1000 },
        "transfer/g2": { balance: 500 },
        "transfer/g3": { balance: 0 }
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
          ] },
        { msg: { sender: "bob", blockTimestamp: 2000, chainId: 1 },
          commands: [
              { contract: "transfer", method: "transfer",
                params: { from: "g2", to: "g3", amount: 50 },
                expect_events: [
                    { contract: "transfer", name: "Transfer",
                      value: { from: "g2", to: "g3", amount: 50, sender: "bob" } }
                ] }
          ] },
        { msg: { sender: "carol", blockTimestamp: 3000, chainId: 1 },
          commands: [
              { contract: "transfer", method: "transfer",
                params: { from: "g3", to: "g1", amount: 25 },
                expect_events: [
                    { contract: "transfer", name: "Transfer",
                      value: { from: "g3", to: "g1", amount: 25, sender: "carol" } }
                ] }
          ] }
    ],
    expected_emits: [
        { contract: "transfer", name: "Transfer", value: { from: "g1", to: "g2", amount: 100, sender: "alice" } },
        { contract: "transfer", name: "Transfer", value: { from: "g2", to: "g3", amount: 50,  sender: "bob" } },
        { contract: "transfer", name: "Transfer", value: { from: "g3", to: "g1", amount: 25,  sender: "carol" } }
    ],
    expected: {
        "transfer/g1": { balance: 925 },
        "transfer/g2": { balance: 550 },
        "transfer/g3": { balance: 25 }
    }
})
