// watchdog_test — изолированный тест scan_expired/execute_gc.

var TTL_SHORT = 2;
var TTL_LONG  = 100000;

// t1: создать short (ttl=2), parent (ttl=long)
registerMethod("t_wd_create", function(p: {}, m: JsiMsg) {
    var g = get("wd_short", TTL_SHORT);   // ← orCreate (не null)
    g.set({ v: 1, ts: m.blockTimestamp });
});

// t2: создать long (ttl=long) — не должен удалиться
registerMethod("t_wd_long", function(p: {}, m: JsiMsg) {
    var g = get("wd_long", TTL_LONG);   // ← orCreate (не null)
    g.set({ v: 2, ts: m.blockTimestamp });
});

// t3: цепочка — parent (ttl=long), child (ttl=short) — parent должен выжить
registerMethod("t_wd_chain", function(p: {}, m: JsiMsg) {
    var g = get("wd_chain", TTL_LONG);   // ← orCreate (не null)
    var c = g.get("wd_child", TTL_SHORT);   // ← orCreate (не null)
    c.set({ v: 3, ts: m.blockTimestamp });
});

// view: получить state гранул (для проверки)
registerView("t_wd_state", function(p: {path: string}) {
    try {
        var g = get(String(p.path));   // ← read-only (null если нет)
        if (!g) return { found: false };   // ← ОБЯЗАТЕЛЬНО (TypeScript)
        return { found: true, value: g.get() };
    } catch (e) {
        return { found: false };
    }
});
