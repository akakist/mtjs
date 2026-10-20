({
    time: 1000,
    initial: {},
    transactions: [
        { msg: { sender: "alice", blockTimestamp: 1000 },
          commands: [
              { contract: "parse", method: "parse",
                params: { s: "10", radix: 1 }, expect_error: true },
              { contract: "parse", method: "parse",
                params: { s: "10", radix: 37 }, expect_error: true },
              { contract: "parse", method: "parse",
                params: { s: "10", radix: -1 }, expect_error: true }
          ] }
    ],
    expected_emits: [],
    expected: {}
})