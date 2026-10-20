({
    initial: {},
    transactions: [
        { contract: "strconcat", method: "concat",
          params: { a: "a\"b", b: "c\\d" },
          expect_result: "a\"bc\\d" },
        { contract: "strconcat", method: "concat",
          params: { a: "line1\n", b: "line2" },
          expect_result: "line1\nline2" },
        { contract: "strconcat", method: "concat",
          params: { a: "tab\t", b: "end" },
          expect_result: "tab\tend" }
    ],
    expected: {}
})