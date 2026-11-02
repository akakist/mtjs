"use strict";
registerMethod("createX", function (p, m) {
    if (getGranule("x"))
        throw new Error("x exists");
    createGranule("x", { v: p.v !== undefined ? Number(p.v) : 1 }, Number(p.ttl));
});
registerMethod("createY", function (p, m) {
    if (getGranule("y"))
        throw new Error("y exists");
    // ttl может быть undefined — проверка в createGranule
    createGranule("y", { v: 1 }, p.ttl !== undefined ? Number(p.ttl) : undefined);
});
registerMethod("createZ", function (p, m) {
    if (getGranule("z"))
        throw new Error("z exists");
    createGranule("z", { v: 1 }, Number(p.ttl));
});
registerMethod("setX", function (p, m) {
    if (!getGranule("x"))
        throw new Error("x not exists");
    setGranule("x", { v: Number(p.v) });
});
registerMethod("deleteX", function (p, m) {
    if (!getGranule("x"))
        throw new Error("x not exists");
    deleteGranule("x");
});
