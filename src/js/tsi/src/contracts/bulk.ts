// bulk.ts — тестовый контракт для нагрузочного теста state (sync, GC, catchup).
//   fill(prefix, count, ttl?)  — создать count гранул prefix/0 .. prefix/(count-1)
//   count(prefix)              — view: сколько гранул существует (0..N)
//   del(prefix, count)         — удалить count гранул (0..count-1)
//   ttl(prefix, i)             — view: ttl гранулы prefix/i (или 0)
//   value(prefix, i)           — view: значение гранулы prefix/i (или null)

var DEFAULT_TTL = 86400;

registerMethod("fill", function(p: { prefix: string, count: number, ttl?: number }, m: JsiMsg) {
    var prefix = String(p.prefix);
    var n = Number(p.count);
    if (!(n > 0)) throw new Error("count must be > 0");
    var ttl = p.ttl !== undefined ? Number(p.ttl) : DEFAULT_TTL;
    if (!(ttl > 0)) throw new Error("ttl must be > 0");
    for (var i = 0; i < n; i++) {
        var g = get(prefix + "/" + String(i), ttl);
        g.set({ i: i, ts: m.blockTimestamp, prefix: prefix });
    }
});

registerMethod("del", function(p: { prefix: string, count: number }, m: JsiMsg) {
    var prefix = String(p.prefix);
    var n = Number(p.count);
    if (!(n > 0)) throw new Error("count must be > 0");
    for (var i = 0; i < n; i++) {
        var g = get(prefix + "/" + String(i));
        if (g) g.del();
    }
});

registerView("count", function(p: { prefix: string }) {
    var prefix = String(p.prefix);
    var n = 0;
    for (var i = 0; ; i++) {
        var g = get(prefix + "/" + String(i));
        if (!g) break;
        n++;
        if (n > 200000) break;   // защита от бесконечности (count в тесте)
    }
    return { count: n };
});


registerView("value", function(p: { prefix: string, i: number }) {
    var g = get(String(p.prefix) + "/" + String(p.i));
    if (!g) return { found: false };
    return { found: true, value: g.get() };
});
