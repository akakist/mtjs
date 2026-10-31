"use strict";
registerMethod("whoCalled", function (p, m) {
    emit("whoCalled", { caller: m.caller, sender: m.sender });
});
registerMethod("onlyA", function (p, m) {
    if (m.caller !== "caller_a")
        throw new Error("only caller_a");
    emit("onlyA", { ok: true });
});
