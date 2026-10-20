registerMethod("concat", function(p) {
    return p.a + p.b;
});
registerMethod("concatNum", function(p) {
    return p.a + p.b + p.c;
});
registerMethod("empty", function(p) {
    return p.a + "";
});