"use strict";
registerMethod("loop", function (p, m) {
    while (true) { }
});
registerMethod("loopN", function (p, m) {
    var n = Number(p.n);
    var x = 0;
    for (var i = 0; i < n; i++) {
        x += i;
    }
    return x;
});
