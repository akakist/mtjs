({
    initial: {},
    transactions: [
        { contract: "strconcat", method: "concat",
          params: { a: 1, b: "2" },
          expect_result: "12" },
        { contract: "strconcat", method: "concat",
          params: { a: "2", b: 1 },
          expect_result: "21" },
        { contract: "strconcat", method: "concat",
          params: { a: "a", b: 1 },
          expect_result: "a1" },
        { contract: "strconcat", method: "concat",
          params: { a: 1, b: "a" },
          expect_result: "1a" }
    ],
    expected: {}
})