"use strict";
// storage_test — JSI_Granule test (get = orCreate с ttl; read = без ttl, null если нет).
var TTL = 86400;
// ============================================================
// t1/t2/t3 — базовые (create/set/get, del, nested)
// ============================================================
registerMethod("t1_create_set_get", function (p, m) {
    var g = get("t1/a", 86400);
    g.set({ hello: "world" });
});
registerMethod("t2_del", function (p, m) {
    var g = get("t2/x", 86400);
    g.set({ v: 1 });
    g.del();
});
registerMethod("t3_nested", function (p, m) {
    var g = get("t3/a", 86400);
    var c = g.get("b", 86400);
    c.set({ n: 42 });
});
// ============================================================
// simple (concert/sectorA/place11) — цепочка
// ============================================================
registerMethod("t_simple_set", function (p, m) {
    var g = get("concert", TTL);
    var s = g.get("sectorA", TTL);
    var pl = s.get("place11", TTL);
    pl.set({ owner: m.sender, price: 100 });
});
registerView("t_simple_get", function (p) {
    var g = get("concert");
    if (!g)
        return { found: false };
    var s = g.get("sectorA");
    if (!s)
        return { found: false };
    var pl = s.get("place11");
    if (!pl)
        return { found: false };
    return pl.get();
});
registerMethod("t_simple_del", function (p, m) {
    var g = get("concert", TTL);
    var s = g.get("sectorA", TTL);
    var pl = s.get("place11", TTL);
    pl.del();
});
// ============================================================
// view guard: мутации в view → throw / not a function
// ============================================================
registerView("t_view_set", function (p) {
    var g = get("concert");
    if (!g)
        return { found: false };
    g.set({ x: 1 }); // view-класс без set → "not a function"
});
registerView("t_view_create", function (p) {
    get("view_cannot_create", TTL); // argc==2 → writable → throw "operation not allowed in view"
});
registerView("t_view_del", function (p) {
    var g = get("concert");
    if (!g)
        return { found: false };
    g.del(); // view-класс без del → "not a function"
});
registerView("t_view_callmethod", function (p) {
    callMethod("storage_test", "t_simple_set", {}); // → throw
});
registerView("t_view_callview", function (p) {
    callView("storage_test", "t_simple_get", {}, 5); // → throw
});
// ============================================================
// GC: ttl=2
// ============================================================
registerMethod("t_short_ttl", function (p, m) {
    var g = get("short", 2);
    g.set({ v: 1 });
});
