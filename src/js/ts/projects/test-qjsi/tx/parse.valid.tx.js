({
    time: 1000,
    initial: {},
    transactions: [
        { msg: { sender: "alice", blockTimestamp: 1000 },
          commands: [
              { contract: "parse", method: "parse", params: { s: "123" }, expect_result: 123 },
              { contract: "parse", method: "parse", params: { s: "123", radix: 10 }, expect_result: 123 },
              { contract: "parse", method: "parse", params: { s: "0" }, expect_result: 0 },
              { contract: "parse", method: "parse", params: { s: "-42" }, expect_result: -42 },
              { contract: "parse", method: "parse", params: { s: "+42" }, expect_result: 42 },
              { contract: "parse", method: "parse", params: { s: "  42  " }, expect_result: 42 }
          ] }
    ],
    expected_emits: [],
    expected: {}
})
