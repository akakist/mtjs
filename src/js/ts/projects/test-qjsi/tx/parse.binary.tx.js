({
    time: 1000,
    initial: {},
    transactions: [
        { msg: { sender: "alice", blockTimestamp: 1000 },
          commands: [
//              { contract: "parse", method: "parseAuto", params: { s: "0b101" }, expect_result: 5 },
//              { contract: "parse", method: "parseAuto", params: { s: "0o17" }, expect_result: 15 },
              { contract: "parse", method: "parseAuto", params: { s: "0xFF" }, expect_result: 255 },
//              { contract: "parse", method: "parseAuto", params: { s: "1_000" }, expect_result: 1000 }
          ] }
    ],
    expected_emits: [],
    expected: {}
})