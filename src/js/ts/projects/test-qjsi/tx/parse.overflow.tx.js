({
    time: 1000,
    initial: {},
    transactions: [
        { msg: { sender: "alice", blockTimestamp: 1000 },
          commands: [
              { contract: "parse", method: "parse",
                params: { s: "9223372036854775808" }, expect_error: true },
              { contract: "parse", method: "parse",
                params: { s: "-9223372036854775809" }, expect_error: true },
              { contract: "parse", method: "parse",
                params: { s: "99999999999999999999999" }, expect_error: true }
          ] }
    ],
    expected_emits: [],
    expected: {}
})
