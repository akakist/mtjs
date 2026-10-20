({
    initial: {},
    transactions: [
        { contract: "strconcat", method: "concat",
          params: { a: "привет", b: "мир" },
          expect_result: "приветмир" },
        { contract: "strconcat", method: "concat",
          params: { a: "😀", b: "😀" },
          expect_result: "😀😀" },
        { contract: "strconcat", method: "concat",
          params: { a: "α", b: "β" },
          expect_result: "αβ" }
    ],
    expected: {}
})