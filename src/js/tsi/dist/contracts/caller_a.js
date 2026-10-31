"use strict";
registerMethod("whoAmI", function (p, m) {
    emit("whoAmI", { caller: m.caller, sender: m.sender });
});
registerMethod("callB", function (p, m) {
    var r = callMethod("caller_b", "whoCalled", {});
});
