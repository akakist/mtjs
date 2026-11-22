#!/usr/local/bin/mtjs
// chk-resurrect.ts — удаление гранулы по TTL (тумбстоун) и повторное создание на том же месте,
// плюс компакция: живая гранула не должна удаляться, тумбстоун — должен.
// (Контракт больше не может явно удалять гранулу — только TTL.)
import * as std from "std";
// JSON.stringify с поддержкой BigInt (значения гранул — 128-битные)
function jstr(v) {
    return JSON.stringify(v, (_k, x) => (typeof x === "bigint" ? Number(x) : x));
}
// гранулы/эмиты хранят целые как BigInt, а тесты сравнивают с обычными числами
function unBig(v) {
    if (typeof v === "bigint")
        return Number(v);
    if (Array.isArray(v))
        return v.map(unBig);
    if (v && typeof v === "object") {
        const o = {};
        for (const k of Object.keys(v))
            o[k] = unBig(v[k]);
        return o;
    }
    return v;
}
const node = "127.0.0.1:2301";
const KEYS_PATH = std.getenv("MTJS_KEYS_PATH") || "keys_test.json";
const SRC = `"use strict";
registerMethod("createX", function (p, m) {
    if (getGranule("x")) throw new Error("x exists");
    createGranule("x", { v: p.v }, p.ttl || 100000);
});
registerMethod("createY", function (p, m) {
    if (getGranule("y")) throw new Error("y exists");
    createGranule("y", { v: p.v }, p.ttl || 100000);
});
`;
let rootSk = "";
async function loadKeys() {
    const raw = await mtjs.fs.readFile(KEYS_PATH);
    const keys = JSON.parse(raw);
    const r = keys.users?.root?.keys?.[0];
    if (!r?.sk)
        throw new Error("keys_test.json: no users.root.keys[0].sk");
    rootSk = r.sk;
}
const GREEN = "\x1b[32m";
const RED = "\x1b[31m";
const RESET = "\x1b[0m";
let pass = 0;
let fail = 0;
function check(c, m) {
    if (c) {
        pass++;
        console.log("  " + GREEN + "✅" + RESET + " " + m);
    }
    else {
        fail++;
        console.log("  " + RED + "❌" + RESET + " " + m);
    }
}
function errStr(e) {
    return (e && typeof e === "object" && ("errcode" in e || "errstr" in e))
        ? "errcode=" + e.errcode + " errstr=" + e.errstr : String(e);
}
const pending = {};
mtjs.tx_subscribe(node, (b) => {
    for (const h of b.tx_hashes) {
        const p = pending[h];
        if (p) {
            delete pending[h];
            clearTimeout(p.timer);
            p.resolve(b);
        }
    }
});
function waitTx(h, to) {
    return new Promise((res, rej) => {
        const timer = setTimeout(() => { delete pending[h]; rej({ errcode: 1, errstr: "timeout" }); }, to * 1000);
        pending[h] = { resolve: res, reject: rej, timer };
    });
}
async function getNonce(addr) {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    return BigInt(ui.nonce);
}
async function getBalance(addr) {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    return BigInt(ui.balance);
}
async function submitAndWait(tx, value = 0, gasLimit = 100000, retries = 5) {
    for (let attempt = 0; attempt < retries; attempt++) {
        const nonce = await getNonce(rootAddr);
        const req = { tx, nonce: nonce.toString(), value, gasLimit, priority: 0, sender: rootAddr, key_id: 0 };
        const rsp = await mtjs.tx_submit(node, 1, jstr(req), rootSk, nonce.toString(), () => { });
        if (rsp.err) {
            if (attempt + 1 < retries) {
                await new Promise(r => setTimeout(r, 300));
                continue;
            }
            throw new Error("submit: " + rsp.err_str);
        }
        const b = await waitTx(rsp.tx_hash, 15);
        const st = b.status(rsp.tx_hash);
        const es = b.errstr(rsp.tx_hash) || "";
        if (st === "err" && es.indexOf("invalid nonce") >= 0) {
            if (attempt + 1 < retries) {
                await new Promise(r => setTimeout(r, 300));
                continue;
            }
            throw new Error("invalid nonce: " + es);
        }
        return { rsp, block: b };
    }
    throw new Error("submitAndWait: retries exhausted");
}
function childEmits(block, txHash) {
    const ch = block.children(txHash);
    const out = {};
    if (ch)
        for (const k of Object.keys(ch))
            out[k] = ch[k].emits;
    return out;
}
function hasEmit(em, name) {
    for (const k of Object.keys(em))
        for (const e of em[k]) {
            if (e[0] !== name)
                continue;
            let v = e[1];
            v = unBig(v);
            return v || {};
        }
    return null;
}
async function granuleState(key) {
    const g = await mtjs.get_contract_data(node, "resurrect_test", key, 3);
    if (!g.found)
        return { found: false };
    let v = null;
    try {
        v = unBig(g.value);
    }
    catch (_) { }
    return { found: true, v };
}
async function waitGone(key, to = 15) {
    const t0 = Date.now();
    while (Date.now() - t0 < to * 1000) {
        try {
            if (!(await granuleState(key)).found)
                return true;
        }
        catch (e) { }
        await new Promise(r => setTimeout(() => r(), 200));
    }
    return false;
}
const rootAddr = mtjs.addr_from_name("root");
async function gcDelete() {
    // TTL (1с) истёк → scan_expired → root.gc удаляет гранулу (тумбстоун)
    await new Promise(r => setTimeout(() => r(), 1500));
    const r = await mtjs.scan_expired(node, "", 5);
    if (r.dbids && r.dbids.length > 0) {
        await submitAndWait([{ contract: "root", method: "gc", params: { items: r.dbids } }], 0, 200000);
    }
}
async function compact(label) {
    const { rsp, block } = await submitAndWait([
        { contract: "root", method: "compact", params: { retention: "1", max_items: 1000 } },
    ], 0, 200000);
    const st = block.status(rsp.tx_hash);
    const em = hasEmit(childEmits(block, rsp.tx_hash), "compact") || {};
    console.log("  " + label + ": status=" + st + " deleted=" + em.deleted + " complete=" + em.complete + " gen=" + em.gen);
    return { st, em };
}
async function main() {
    await loadKeys();
    console.log("=== RESURRECT (node " + node + ") ===");
    const rb = await getBalance(rootAddr);
    if (rb < 50000000n) {
        if (rb < 200000n) {
            check(false, "root low — clean db");
            return;
        }
        const { rsp, block } = await submitAndWait([
            { contract: "root", method: "mint", params: { amount: "100000000" } },
        ], 0, 20000);
        check(block.status(rsp.tx_hash) === "ok", "mint root");
    }
    const c = await mtjs.get_contract(node, "resurrect_test", 3);
    if (c.found) {
        const { rsp, block } = await submitAndWait([
            { contract: "root", method: "contract_update", params: { name: "resurrect_test", src: SRC } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "contract_update (errstr=" + block.errstr(rsp.tx_hash) + ")");
    }
    else {
        const { rsp, block } = await submitAndWait([
            { contract: "root", method: "contract_deploy", params: { name: "resurrect_test", src: SRC, ttl: 100000 } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "contract_deploy (errstr=" + block.errstr(rsp.tx_hash) + ")");
    }
    {
        console.log("\n[t1] create(v=1, ttl=1) → TTL → create(v=2)");
        const { rsp: r1, block: b1 } = await submitAndWait([{ contract: "resurrect_test", method: "createX", params: { v: 1, ttl: 1 } }]);
        check(b1.status(r1.tx_hash) === "ok", "create v=1 ok (errstr=" + b1.errstr(r1.tx_hash) + ")");
        await gcDelete();
        check(await waitGone("x"), "x deleted по TTL (found=false)");
        const { rsp: r2, block: b2 } = await submitAndWait([{ contract: "resurrect_test", method: "createX", params: { v: 2 } }]);
        check(b2.status(r2.tx_hash) === "ok", "create заново ok (errstr=" + b2.errstr(r2.tx_hash) + ")");
        const st = await granuleState("x");
        check(!!st.v && st.v.v === 2, "значение новое (v=" + (st.v ? st.v.v : "?") + ", ждали 2)");
    }
    {
        console.log("\n[t2] компакция: живая гранула не удаляется");
        const { st } = await compact("compact");
        check(st === "ok", "compact status ok");
        await new Promise(r => setTimeout(() => r(), 300));
        const s = await granuleState("x");
        check(s.found, "x жив после компакции");
        check(!!s.v && s.v.v === 2, "значение всё ещё 2");
    }
    {
        console.log("\n[t3] TTL-удаление → компакция → y отсутствует, блоки идут");
        const { rsp: r1, block: b1 } = await submitAndWait([{ contract: "resurrect_test", method: "createY", params: { v: 1, ttl: 1 } }]);
        check(b1.status(r1.tx_hash) === "ok", "createY ok");
        await gcDelete();
        check(await waitGone("y"), "y deleted по TTL");
        const { st } = await compact("compact");
        check(st === "ok", "compact status ok");
        await new Promise(r => setTimeout(() => r(), 300));
        check(!(await granuleState("y")).found, "y отсутствует после чистки тумбстоуна");
        const { rsp: r2, block: b2 } = await submitAndWait([{ contract: "resurrect_test", method: "createY", params: { v: 3 } }]);
        check(b2.status(r2.tx_hash) === "ok", "сеть жива: createY(v=3) ok");
        const s3 = await granuleState("y");
        check(s3.found && s3.v && s3.v.v === 3, "y снова создан");
    }
    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    console.log(fail === 0 ? GREEN + "DONE" + RESET : RED + "FAIL" + RESET);
    std.exit(fail);
}
main().catch((e) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => { }, 1000000);
