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
// view: бесконечный цикл — прерывается view_interrupt_handler по timeout_sec
registerView("loopView", function (p) {
    while (true) { }
});
// view: ограниченный цикл — должен вернуть результат (без прерывания)
registerView("loopViewN", function (p) {
    var n = Number(p.n);
    var x = 0;
    for (var i = 0; i < n; i++) {
        x += i;
    }
    return x;
});
