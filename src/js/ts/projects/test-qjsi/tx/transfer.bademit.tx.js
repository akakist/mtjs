({
    time: 1000,
    initial: {},
    transactions: [
        { msg: { sender: "alice", blockTimestamp: 1000 },
          commands: [
              { contract: "observer", method: "callBadEmit",
                params: {}, expect_error: true }
          ] }
    ],
    expected_emits: [],
    expected: {}
})