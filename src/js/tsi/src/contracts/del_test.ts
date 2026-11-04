registerMethod("createX", function(p: {}, m: JsiMsg) {
    if (getGranule("x")) throw new Error("x exists");
    createGranule("x", { v: 1 }, 100000);
});
registerMethod("deleteX", function(p: {}, m: JsiMsg) {
    if (!getGranule("x")) throw new Error("x not exists");
    deleteGranule("x");
});
registerMethod("fail", function(p: {}, m: JsiMsg) {
    throw new Error("intentional failure");
});
