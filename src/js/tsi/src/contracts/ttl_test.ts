registerMethod("createX", function(p: { ttl: number, v?: number }, m: JsiMsg) {
    if (getGranule("x")) throw new Error("x exists");
    createGranule("x", { v: p.v !== undefined ? Number(p.v) : 1 }, Number(p.ttl));
});

registerMethod("createY", function(p: { ttl?: number }, m: JsiMsg) {
    if (getGranule("y")) throw new Error("y exists");
    // ttl может быть undefined — проверка в createGranule
    createGranule("y", { v: 1 }, p.ttl !== undefined ? Number(p.ttl) : undefined as any);
});

registerMethod("createZ", function(p: { ttl: number }, m: JsiMsg) {
    if (getGranule("z")) throw new Error("z exists");
    createGranule("z", { v: 1 }, Number(p.ttl));
});

registerMethod("setX", function(p: { v: number }, m: JsiMsg) {
    if (!getGranule("x")) throw new Error("x not exists");
    setGranule("x", { v: Number(p.v) });
});

registerMethod("deleteX", function(p: {}, m: JsiMsg) {
    if (!getGranule("x")) throw new Error("x not exists");
    deleteGranule("x");
});