({
    initial: {},
    transactions: [
        { contract: "strconcat", method: "concat",
          params: { a: "hello", b: "world" },
          expect_result: "helloworld" },
        { contract: "strconcat", method: "concat",
          params: { a: "", b: "" },
          expect_result: "" },
        { contract: "strconcat", method: "concat",
          params: { a: "a", b: "" },
          expect_result: "a" },
        { contract: "strconcat", method: "concat",
          params: { a: "", b: "b" },
          expect_result: "b" }
    ],
    expected: {}
})