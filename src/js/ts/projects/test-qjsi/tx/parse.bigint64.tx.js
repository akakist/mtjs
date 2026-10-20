({
    time: 1000,
    initial: {},
    transactions: [
        { msg: { sender: "alice", blockTimestamp: 1000 },
          commands: [
              { contract: "parse", method: "parse",
                params: { s: "9223372036854775807" },
                expect_result: 9223372036854775807 },
              { contract: "parse", method: "parse",
                params: { s: "9007199254740993" },
                expect_result: 9007199254740993 },
              { contract: "parse", method: "parse",
                params: { s: "-9223372036854775807" },
                expect_result: -9223372036854775807 }
          ] }
    ],
    expected_emits: [],
    expected: {}
})