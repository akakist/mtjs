({
    time: 1000,
    initial: {},
    transactions: [
        { msg: { sender: "alice", blockTimestamp: 1000 },
          commands: [
              { contract: "gym", method: "init", params: { admin: "alice" } },
              { contract: "gym", method: "getAdmin", params: {}, expect_result: { addr: "alice" } },

              { contract: "gym", method: "issuePass", params: { visits: 10 } },
              { contract: "gym", method: "getPass", params: { user: "alice" },
                expect_result: { found: true, visits: 10 } },

              { contract: "gym", method: "createEntry", params: { M: 3, ttl: 300 } },
              { contract: "gym", method: "getPass", params: { user: "alice" },
                expect_result: { found: true, visits: 7 } },
              { contract: "gym", method: "getEntry", params: { user: "alice" },
                expect_result: { found: true, id: 0, M: 3, used: 0, disabled: false, valid: true } },

              { contract: "gym", method: "createEntry", params: { M: 3, ttl: 300 }, expect_error: true },

              { contract: "gym", method: "markUsed", params: { user: "alice", entry_id: 0, count: 1 } },
              { contract: "gym", method: "getEntry", params: { user: "alice" },
                expect_result: { found: true, id: 0, M: 3, used: 1, disabled: false, valid: true } },

              { contract: "gym", method: "markUsed", params: { user: "alice", entry_id: 0, count: 2 } },
              { contract: "gym", method: "getEntry", params: { user: "alice" },
                expect_result: { found: false } },

              { contract: "gym", method: "markUsed", params: { user: "alice", entry_id: 0, count: 1 }, expect_error: true },

              { contract: "gym", method: "createEntry", params: { M: 3, ttl: 300 } },
              { contract: "gym", method: "getPass", params: { user: "alice" },
                expect_result: { found: true, visits: 4 } },
              { contract: "gym", method: "getEntry", params: { user: "alice" },
                expect_result: { found: true, id: 1, M: 3, used: 0, disabled: false, valid: true } },
              { contract: "gym", method: "listEntries", params: { user: "alice" },
                expect_result: [ { id: 1, M: 3, used: 0, disabled: false, valid: true } ] },

              { contract: "gym", method: "cancelEntry", params: { user: "alice", entry_id: 1 } },
              { contract: "gym", method: "getPass", params: { user: "alice" },
                expect_result: { found: true, visits: 7 } },
              { contract: "gym", method: "getEntry", params: { user: "alice" },
                expect_result: { found: false } }
          ] },
        { msg: { sender: "bob", blockTimestamp: 1000 },
          commands: [
              { contract: "gym", method: "issuePass", params: { visits: 5, user: "bob" }, expect_error: true },
              { contract: "gym", method: "createEntry", params: { M: 1, ttl: 300 }, expect_error: true },
              { contract: "gym", method: "markUsed", params: { user: "alice", entry_id: 0, count: 1 }, expect_error: true },
              { contract: "gym", method: "cancelEntry", params: { user: "alice", entry_id: 0 }, expect_error: true },
              { contract: "gym", method: "disableEntry", params: { user: "alice", entry_id: 0 }, expect_error: true }
          ] }
    ],
    expected_emits: [],
    expected: {}
} as TestTx);
