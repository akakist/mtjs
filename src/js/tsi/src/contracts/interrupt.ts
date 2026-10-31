registerMethod("loop", function (p: {}, m: JsiMsg) {
    while (true) { }
});
registerMethod("loopN", function (p: { n: number }, m: JsiMsg) {
    var n = Number(p.n);
    var x = 0;
    for (var i = 0; i < n; i++) { x += i; }
    return x;
});
