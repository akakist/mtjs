#!/usr/local/bin/mtjs
import * as std from "std";
const node = "127.0.0.1:2301";
const WD_SRC = "/home/bsr/mtjs/src/js/tsi/dist/contracts/watchdog_test.js";
const KEYS_PATH = std.getenv("MTJS_KEYS_PATH") || "keys_test.json";
const GAS_LIMIT = 100000;
let rootSk = "";
let rootPk = "";
async function loadKeys() {
    const raw = await mtjs.fs.readFile(KEYS_PATH);
    const keys = JSON.parse(raw);
    const r = keys.users?.root?.keys?.[0];
    if (!r?.sk || !r?.pk)
        throw new Error("keys_test.json: no users.root.keys[0]");
    rootSk = r.sk;
    rootPk = r.pk;
}
const GREEN = "\x1b[32m";
const RED = "\x1b[31m";
const RESET = "\x1b[0m";
let pass = 0;
let fail = 0;
function check(cond, msg) {
    if (cond) {
        pass++;
        console.log("  " + GREEN + "✅" + RESET + " " + msg);
    }
    else {
        fail++;
        console.log("  " + RED + "❌" + RESET + " " + msg);
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
async function submitAndWait(sk, addr, tx, value = 0, gasLimit = GAS_LIMIT, retries = 5) {
    for (let a = 0; a < retries; a++) {
        const nonce = await getNonce(addr);
        const req = { tx, nonce: nonce.toString(), value, gasLimit, priority: 0, sender: addr, key_id: 0 };
        const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, nonce.toString(), () => { });
        if (rsp.err) {
            if (a + 1 < retries) {
                await new Promise(r => setTimeout(r, 300));
                continue;
            }
            throw new Error("submit: " + rsp.err_str);
        }
        const b = await waitTx(rsp.tx_hash, 10);
        const st = b.status(rsp.tx_hash);
        const es = b.errstr(rsp.tx_hash) || "";
        if (st === "err" && es.indexOf("invalid nonce") >= 0) {
            if (a + 1 < retries) {
                await new Promise(r => setTimeout(r, 300));
                continue;
            }
            throw new Error("invalid nonce: " + es);
        }
        return { rsp, block: b, nonce };
    }
    throw new Error("submitAndWait: retries exhausted");
}
const rootAddr = mtjs.addr_from_name("root");
async function ensureRootFunded(minBalance) {
    const ui = await mtjs.get_user_nonce(node, rootAddr, 3);
    const bal = BigInt(ui.balance);
    if (bal >= minBalance)
        return;
    if (bal < 200000n) {
        check(false, "root too low");
        return;
    }
    await submitAndWait(rootSk, rootAddr, [
        { contract: "root", method: "mint", params: { amount: "100000000" } },
    ], 0, 20000);
}
async function ensureWd(src) {
    const c = await mtjs.get_contract(node, "watchdog_test", 3);
    const method = c.found ? "contract_update" : "contract_deploy";
    await submitAndWait(rootSk, rootAddr, [
        { contract: "root", method, params: { name: "watchdog_test", src, ttl: 86400 } },
    ]);
}
// scan_expired (один проход, с логом)
async function scanOnce(label) {
    const r = await mtjs.scan_expired(node, "", 5);
    console.log("  [" + label + "] scanned=" + r.scanned + " dbids=" + r.dbids.length + " done=" + r.done);
    return r;
}
// gc (один проход, с логом emits)
async function gcOnce(label, dbids) {
    const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
        { contract: "root", method: "gc", params: { items: dbids } },
    ]);
    check(block.status(rsp.tx_hash) === "ok", label + " gc ok (errstr=" + (block.errstr(rsp.tx_hash) || "") + ")");
    let applied = 0, granule = 0, contract = 0, node = 0, address = 0;
    const em = block.children(rsp.tx_hash);
    if (em)
        for (const k of Object.keys(em)) {
            for (const e of em[k].emits) {
                let v = e[1];
                if (typeof v === "string")
                    try {
                        v = JSON.parse(v);
                    }
                    catch (_) { }
                if (e[0] === "gc" && v && typeof v.applied === "number")
                    applied += v.applied;
                if (e[0] === "gc_granule" && v && typeof v.count === "number")
                    granule += v.count;
                if (e[0] === "gc_contract" && v && typeof v.count === "number")
                    contract += v.count;
                if (e[0] === "gc_node" && v && typeof v.count === "number")
                    node += v.count;
                if (e[0] === "gc_address" && v && typeof v.count === "number")
                    address += v.count;
            }
        }
    console.log("  [" + label + "] applied=" + applied + " granule=" + granule + " contract=" + contract + " node=" + node + " address=" + address);
    return { applied, granule, contract, node, address };
}
// helper: вызывает view t_wd_state и парсит result (а не RPC found/error)
async function wdState(path) {
    const r = await mtjs.call_view(node, "watchdog_test", "t_wd_state", { path }, 5, 5);
    if (!r.found || r.error)
        return { found: false };
    try {
        return JSON.parse(r.result);
    }
    catch (_) {
        return { found: false };
    }
}
async function main() {
    await loadKeys();
    console.log("=== WATCHDOG (node " + node + ") ===");
    await ensureRootFunded(50000000n);
    const src = await mtjs.fs.readFile(WD_SRC);
    await ensureWd(src);
    // ============================================================
    // [t1] scan_expired (без создания) — базовый
    // ============================================================
    {
        console.log("\n[t1] scan_expired (base)");
        const r = await scanOnce("t1");
        check(typeof r.scanned === "number", "scanned is number");
        check(Array.isArray(r.dbids), "dbids is array");
        check(typeof r.done === "boolean", "done is boolean");
    }
    // ============================================================
    // [t2] create short (ttl=2) → wait 3s → scan → gc → deleted
    // ============================================================
    {
        console.log("\n[t2] short ttl=2 → wait 3s → scan → gc");
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "watchdog_test", method: "t_wd_create", params: {} },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "t_wd_create ok (errstr=" + (block.errstr(rsp.tx_hash) || "") + ")");
        const g0 = await wdState("wd_short");
        check(g0.found, "wd_short found (before gc)");
        await new Promise(r => setTimeout(r, 3000));
        const scan = await scanOnce("t2 scan");
        check(scan.dbids.length > 0, "scan dbids > 0 (got " + scan.dbids.length + ")");
        if (scan.dbids.length > 0) {
            const g = await gcOnce("t2 gc", scan.dbids);
            check(g.applied > 0, "gc applied > 0 (got " + g.applied + ")");
            check(g.granule > 0, "gc_granule > 0 (got " + g.granule + ")");
        }
        // wait InvalidateRoot (GrainReader root перечитан)
        await new Promise(r => setTimeout(r, 2000));
        const g1 = await wdState("wd_short");
        check(g1.found === false, "wd_short deleted (view found=false)");
    }
    // ============================================================
    // [t3] create long (ttl=100000) → wait 3s → scan → gc → NOT deleted
    // ============================================================
    {
        console.log("\n[t3] long ttl=100000 → wait 3s → scan → gc → NOT deleted");
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "watchdog_test", method: "t_wd_long", params: {} },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "t_wd_long ok");
        const g0 = await wdState("wd_long");
        check(g0.found, "wd_long found (before gc)");
        await new Promise(r => setTimeout(r, 3000));
        const scan = await scanOnce("t3 scan");
        if (scan.dbids.length > 0)
            await gcOnce("t3 gc", scan.dbids);
        await new Promise(r => setTimeout(r, 2000));
        const g1 = await wdState("wd_long");
        check(g1.found === true, "wd_long still found (ttl long)");
    }
    // ============================================================
    // [t4] chain (parent ttl=long, child ttl=short) → wait 3s → scan → gc
    //      → child deleted, parent alive
    // ============================================================
    {
        console.log("\n[t4] chain (parent ttl=long, child ttl=short) → scan → gc");
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "watchdog_test", method: "t_wd_chain", params: {} },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "t_wd_chain ok (errstr=" + (block.errstr(rsp.tx_hash) || "") + ")");
        const gp0 = await wdState("wd_chain");
        const gc0 = await wdState("wd_chain/wd_child");
        check(gp0.found, "wd_chain found (parent)");
        check(gc0.found, "wd_chain/wd_child found (child)");
        await new Promise(r => setTimeout(r, 3000));
        const scan = await scanOnce("t4 scan");
        check(scan.dbids.length > 0, "scan dbids > 0 (got " + scan.dbids.length + ")");
        if (scan.dbids.length > 0) {
            const g = await gcOnce("t4 gc", scan.dbids);
            check(g.granule > 0, "gc_granule > 0 (child)");
        }
        // wait InvalidateRoot (GrainReader root перечитан)
        await new Promise(r => setTimeout(r, 2000));
        const gp1 = await wdState("wd_chain");
        const gc1 = await wdState("wd_chain/wd_child");
        check(gc1.found === false, "wd_chain/wd_child deleted (child expired)");
        check(gp1.found === true, "wd_chain still found (parent ttl long)");
    }
    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0)
        console.log(GREEN + "DONE" + RESET);
    else
        console.log(RED + "FAIL" + RESET);
    std.exit(fail);
}
main().catch((e) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => { }, 1000000);
