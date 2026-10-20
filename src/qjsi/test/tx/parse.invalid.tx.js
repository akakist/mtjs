({
    time: 1000,
    initial: {},
    transactions: [
        { msg: { sender: "alice", blockTimestamp: 1000 },
          commands: [
              { contract: "parse", method: "parse",
                params: { s: "abc" }, expect_error: true },
              { contract: "parse", method: "parse",
                params: { s: "" }, expect_error: true },
              { contract: "parse", method: "parse",
                params: { s: "1.5" }, expect_error: true },
              { contract: "parse", method: "parse",
                params: { s: "1e10" }, expect_error: true },
              { contract: "parse", method: "parse",
                params: { s: "0b101" }, expect_error: true },
              { contract: "parse", method: "parse",
                params: { s: "0o17" }, expect_error: true },
              { contract: "parse", method: "parse",
                params: { s: "42abc" }, expect_error: true },
              { contract: "parse", method: "parse",
                params: { s: "abc42" }, expect_error: true },
              { contract: "parse", method: "parse",
                params: { s: "12 34" }, expect_error: true },
              { contract: "parse", method: "parse",
                params: { s: "NaN" }, expect_error: true },
              { contract: "parse", method: "parse",
                params: { s: "Infinity" }, expect_error: true },
              { contract: "parse", method: "parse",
                params: { s: "-Infinity" }, expect_error: true }
          ] }
    ],
    expected_emits: [],
    expected: {}
})