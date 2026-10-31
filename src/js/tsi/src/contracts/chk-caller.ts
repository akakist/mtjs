registerMethod("callOnlyA", function (p, m) {
    return callMethod("caller_b", "onlyA", {});
});
