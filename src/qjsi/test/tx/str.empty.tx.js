({
    initial: {},
    transactions: [
        { contract: "strconcat", method: "empty",
          params: { a: "hello" },
          expect_result: "hello" },
        { contract: "strconcat", method: "empty",
          params: { a: "" },
          expect_result: "" }
    ],
    expected: {}
})