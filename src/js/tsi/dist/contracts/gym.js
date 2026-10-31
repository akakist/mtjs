"use strict";
// gym — JSI-контракт (tsc → tsi/dist/contracts/gym.js)
//   admin                = { addr }
//   abonement<user>      = { visits }
//   entry<user>          = { next_id, passes: [ { id, M, used, created_at, expires_at, disabled } ] }
// value не используется (фиат оффчейн; issuePass — admin).
// Время — только в method (m.blockTimestamp). View без времени.
var PRICE_PER_VISIT = 10;
function _now(m) { return m.blockTimestamp; }
function _day(ts) { return ts / 86400; }
function _passKey(u) { return "abonement" + u; }
function _entryKey(u) { return "entry" + u; }
function _isAdmin(m) {
    var a = getGranule("admin");
    if (!a)
        return false;
    return String(a.addr) === String(m.sender);
}
registerMethod("init", function (p, m) {
    if (getGranule("admin"))
        throw new Error("already init");
    createGranule("admin", { addr: String(p.admin) }, 0);
});
registerMethod("issuePass", function (p, m) {
    if (!_isAdmin(m))
        throw new Error("admin only");
    var user = p.user ? String(p.user) : String(m.sender);
    var n = Number(p.visits);
    if (!(n > 0))
        throw new Error("bad visits");
    var k = _passKey(user);
    var g = getGranule(k);
    if (g) {
        setGranule(k, { visits: Number(g.visits) + n });
    }
    else {
        createGranule(k, { visits: n }, 86400);
    }
    emit("PassIssued", { user: user, visits: n });
});
registerMethod("buyVisits", function (p, m) {
    var n = Number(p.visits);
    if (!(n > 0))
        throw new Error("bad visits");
    var total = n * PRICE_PER_VISIT;
    consumeValue(total); // явно списать value (нативный токен)
    var user = String(m.sender);
    var k = _passKey(user);
    var g = getGranule(k);
    if (g)
        setGranule(k, { visits: Number(g.visits) + n });
    else
        createGranule(k, { visits: n }, 86400);
    emit("PassIssued", { user: user, visits: n, paid: total });
});
registerMethod("createEntry", function (p, m) {
    var sender = String(m.sender);
    var now = _now(m);
    var M = Number(p.M), ttl = Number(p.ttl);
    if (!(M > 0))
        throw new Error("bad M");
    if (!(ttl > 0))
        throw new Error("bad ttl");
    var ak = _passKey(sender);
    var pass = getGranule(ak);
    if (!pass)
        throw new Error("no pass");
    if (Number(pass.visits) < M)
        throw new Error("no visits");
    var ek = _entryKey(sender);
    var e = getGranule(ek);
    if (!e) {
        e = { next_id: 0, passes: [] };
        createGranule(ek, e, 86400);
    }
    // один активный entry (disabled=false, used<M, expires_at > now) — политика
    for (var i = 0; i < e.passes.length; i++) {
        var pp = e.passes[i];
        if (!pp.disabled && Number(pp.used) < Number(pp.M) && now <= Number(pp.expires_at))
            throw new Error("entry active");
    }
    setGranule(ak, { visits: Number(pass.visits) - M });
    var id = Number(e.next_id);
    e.next_id = id + 1;
    e.passes.push({ id: id, M: M, used: 0, created_at: now, expires_at: now + ttl, disabled: false });
    setGranule(ek, e);
    emit("EntryCreated", { user: sender, id: id, M: M, ttl: ttl });
});
registerMethod("markUsed", function (p, m) {
    if (!_isAdmin(m))
        throw new Error("admin only");
    if (!p.user)
        throw new Error("user required");
    var user = String(p.user);
    var ek = _entryKey(user);
    var e = getGranule(ek);
    if (!e || !e.passes)
        throw new Error("no entry");
    var id = Number(p.entry_id);
    var idx = -1;
    for (var i = 0; i < e.passes.length; i++)
        if (Number(e.passes[i].id) === id) {
            idx = i;
            break;
        }
    if (idx < 0)
        throw new Error("no entry id=" + id);
    var pp = e.passes[idx];
    var now = _now(m);
    if (pp.disabled)
        throw new Error("entry disabled");
    if (now > Number(pp.expires_at))
        throw new Error("entry expired");
    var cnt = Number(p.count ? p.count : 1);
    if (!(cnt > 0))
        throw new Error("bad count");
    var used = Number(pp.used) + cnt;
    if (used > Number(pp.M))
        throw new Error("entry exhausted used=" + used + " M=" + pp.M);
    pp.used = used;
    if (used === Number(pp.M)) {
        e.passes.splice(idx, 1);
        setGranule(ek, e);
        emit("EntryClosed", { user: user, id: id });
    }
    else {
        setGranule(ek, e);
        emit("EntryUsed", { user: user, id: id, used: used, M: Number(pp.M) });
    }
});
registerMethod("cancelEntry", function (p, m) {
    var user = String(p.user);
    var ek = _entryKey(user);
    var e = getGranule(ek);
    if (!e || !e.passes)
        throw new Error("no entry");
    var id = Number(p.entry_id);
    var idx = -1;
    for (var i = 0; i < e.passes.length; i++)
        if (Number(e.passes[i].id) === id) {
            idx = i;
            break;
        }
    if (idx < 0)
        throw new Error("no entry id=" + id);
    var pp = e.passes[idx];
    if (String(user) !== String(m.sender))
        throw new Error("not owner");
    if (pp.disabled)
        throw new Error("entry disabled");
    var refund = Number(pp.M) - Number(pp.used);
    if (refund > 0) {
        var ak = _passKey(user);
        var pass = getGranule(ak);
        if (pass)
            setGranule(ak, { visits: Number(pass.visits) + refund });
    }
    e.passes.splice(idx, 1);
    setGranule(ek, e);
    emit("EntryCancelled", { user: user, id: id, refund: refund });
});
registerMethod("disableEntry", function (p, m) {
    var user = String(p.user);
    var ek = _entryKey(user);
    var e = getGranule(ek);
    if (!e || !e.passes)
        throw new Error("no entry");
    var id = Number(p.entry_id);
    var idx = -1;
    for (var i = 0; i < e.passes.length; i++)
        if (Number(e.passes[i].id) === id) {
            idx = i;
            break;
        }
    if (idx < 0)
        throw new Error("no entry id=" + id);
    if (String(user) !== String(m.sender))
        throw new Error("not owner");
    e.passes[idx].disabled = true;
    setGranule(ek, e);
    emit("EntryDisabled", { user: user, id: id });
});
registerMethod("setAdmin", function (p, m) {
    if (!_isAdmin(m))
        throw new Error("admin only");
    setGranule("admin", { addr: String(p.addr) });
});
registerView("getAdmin", function () {
    var a = getGranule("admin");
    return a ? { addr: a.addr } : { addr: "" };
});
registerView("getPass", function (p) {
    var g = getGranule(_passKey(String(p.user)));
    return g ? { found: true, visits: g.visits } : { found: false };
});
registerView("getEntry", function (p) {
    var ek = _entryKey(String(p.user));
    var e = getGranule(ek);
    if (!e || !e.passes || e.passes.length === 0)
        return { found: false };
    var pp = null;
    if (p.id !== undefined && p.id !== null) {
        var id = Number(p.id);
        for (var i = 0; i < e.passes.length; i++)
            if (Number(e.passes[i].id) === id) {
                pp = e.passes[i];
                break;
            }
    }
    else {
        for (var i = e.passes.length - 1; i >= 0; i--) {
            var x = e.passes[i];
            if (!x.disabled && Number(x.used) < Number(x.M)) {
                pp = x;
                break;
            }
        }
        if (!pp)
            pp = e.passes[e.passes.length - 1];
    }
    if (!pp)
        return { found: false };
    var dis = !!pp.disabled;
    var valid = (!dis && Number(pp.used) < Number(pp.M));
    return { found: true, id: pp.id, M: pp.M, used: pp.used,
        disabled: dis, valid: valid,
        created_at: pp.created_at, expires_at: pp.expires_at };
});
registerView("listEntries", function (p) {
    var ek = _entryKey(String(p.user));
    var e = getGranule(ek);
    if (!e || !e.passes)
        return [];
    var out = [];
    for (var i = 0; i < e.passes.length; i++) {
        var pp = e.passes[i];
        out.push({ id: pp.id, M: pp.M, used: pp.used,
            disabled: !!pp.disabled, valid: (!pp.disabled && Number(pp.used) < Number(pp.M)),
            created_at: pp.created_at, expires_at: pp.expires_at });
    }
    return out;
});
registerView("listPasses", function (p) {
    var idx = getGranule("passes" + String(p.user));
    if (!idx)
        return [];
    var out = [];
    for (var i = 0; i < idx.ids.length; i++) {
        out.push({ day: idx.ids[i], visits: 0 });
    }
    return out;
});
registerMethod("withdraw", function (p, m) {
    if (!_isAdmin(m))
        throw new Error("admin only");
    var amount = Number(p.amount);
    if (!(amount > 0))
        throw new Error("bad amount");
    refundValue(amount); // contract.balance -= amount (guard) → sender value
    emit("Withdraw", { user: String(m.sender), amount: amount });
});
