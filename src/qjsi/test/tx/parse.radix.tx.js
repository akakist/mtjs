({
    time: 1000,
    initial: {},
    transactions: [
        { msg: { sender: "alice", blockTimestamp: 1000 },
          commands: [
              { contract: "parse", method: "parse", params: { s: "ff", radix: 16 }, expect_result: 255 },
              { contract: "parse", method: "parse", params: { s: "0xff", radix: 16 }, expect_result: 255 },
              { contract: "parse", method: "parse", params: { s: "101", radix: 2 }, expect_result: 5 },
              { contract: "parse", method: "parse", params: { s: "777", radix: 8 }, expect_result: 511 },
              { contract: "parse", method: "parse", params: { s: "0x1F", radix: 0 }, expect_result: 31 }
          ] }
    ],
    expected_emits: [],
    expected: {}
})
