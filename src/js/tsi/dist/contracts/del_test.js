"use strict";
registerMethod("createX", function (p, m) {
    if (getGranule("x"))
        throw new Error("x exists");
    createGranule("x", { v: 1 }, 0);
});
registerMethod("deleteX", function (p, m) {
    if (!getGranule("x"))
        throw new Error("x not exists");
    deleteGranule("x");
});
registerMethod("fail", function (p, m) {
    throw new Error("intentional failure");
});
