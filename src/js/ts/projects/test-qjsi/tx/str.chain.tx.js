({
    initial: {},
    transactions: [
        { contract: "strconcat", method: "concatNum",
          params: { a: "a", b: "b", c: "c" },
          expect_result: "abc" },
        { contract: "strconcat", method: "concatNum",
          params: { a: "1", b: "2", c: "3" },
          expect_result: "123" }
    ],
    expected: {}
})