#!/usr/local/bin/mtjs
import * as std from "std";
const node = "127.0.0.1:2301";
const STORAGE_SRC_PATH = "/home/bsr/mtjs/src/js/tsi/dist/contracts/storage_test.js";
const GAS_LIMIT = 100000;
const KEYS_PATH = std.getenv("MTJS_KEYS_PATH") || "keys_test.json";
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
async function submitAndWaitErr(sk, addr, tx, value = 0, gasLimit = GAS_LIMIT, retries = 3) {
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
    throw new Error("submitAndWaitErr: retries exhausted");
}
const rootAddr = mtjs.addr_from_name("root");
async function ensureRootFunded(minBalance) {
    const ui = await mtjs.get_user_nonce(node, rootAddr, 3);
    const bal = BigInt(ui.balance);
    console.log("  root balance:", bal.toString());
    if (bal >= minBalance)
        return;
    if (bal < 200000n) {
        check(false, "root too low for mint");
        return;
    }
    const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
        { contract: "root", method: "mint", params: { amount: "100000000" } },
    ], 0, 20000);
    check(block.status(rsp.tx_hash) === "ok", "mint root 100M ok");
}
async function ensureStorageTest(src) {
    const c = await mtjs.get_contract(node, "storage_test", 3);
    const method = c.found ? "contract_update" : "contract_deploy";
    const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
        { contract: "root", method, params: { name: "storage_test", src, ttl: 86400 } },
    ]);
    check(block.status(rsp.tx_hash) === "ok", "storage_test " + method + " ok (errstr=" + (block.errstr(rsp.tx_hash) || "") + ")");
}
function viewGuardMsg(r) {
    return r.result.indexOf("not allowed in view") >= 0 ||
        r.result.indexOf("not a function") >= 0 ||
        r.result.indexOf("is not a function") >= 0;
}
async function main() {
    await loadKeys();
    console.log("=== STORAGE SIMPLE (node " + node + ") ===");
    await ensureRootFunded(50000000n);
    let src = "";
    try {
        src = await mtjs.fs.readFile(STORAGE_SRC_PATH);
    }
    catch (e) {
        check(false, "read src: " + errStr(e));
        return;
    }
    check(src.length > 0, "storage_test src read (" + src.length + " bytes)");
    await ensureStorageTest(src);
    // cleanup old
    {
        const r = await mtjs.get_contract_data(node, "storage_test", "concert/sectorA/place11", 3);
        if (r.found) {
            await submitAndWait(rootSk, rootAddr, [{ contract: "storage_test", method: "t_simple_del", params: {} }]);
        }
    }
    // ============================================================
    // [t1] method: get_granule(ttl) цепочка + set
    // ============================================================
    {
        console.log("\n[t1] t_simple_set (method) — get_granule(ttl) + set");
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "storage_test", method: "t_simple_set", params: {} },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "t_simple_set ok (errstr=" + (block.errstr(rsp.tx_hash) || "") + ")");
        const r = await mtjs.get_contract_data(node, "storage_test", "concert/sectorA/place11", 3);
        check(r.found === true, "concert/sectorA/place11 found");
        if (r.found) {
            const v = JSON.parse(r.value);
            check(v.owner === rootAddr, "owner == root (got " + v.owner + ")");
            check(v.price === 100, "price == 100 (got " + v.price + ")");
        }
    }
    // ============================================================
    // [t2] view: get_granule() (без ttl) — read ok
    // ============================================================
    {
        console.log("\n[t2] t_simple_get (view) — read ok");
        const r = await mtjs.call_view(node, "storage_test", "t_simple_get", {}, 5, 5);
        console.log("  result:", JSON.stringify(r));
        check(r.found === true, "found");
        check(r.error === false, "no error");
        if (r.found && !r.error) {
            const v = JSON.parse(r.result);
            check(v.owner === rootAddr, "owner == root");
            check(v.price === 100, "price == 100");
        }
    }
    // ============================================================
    // [t3] view guard: set → not a function
    // ============================================================
    {
        console.log("\n[t3] t_view_set (view) → not allowed / not a function");
        const r = await mtjs.call_view(node, "storage_test", "t_view_set", {}, 5, 5);
        console.log("  result:", JSON.stringify(r));
        check(r.found === true, "found (view exists)");
        check(r.error === true, "error=true");
        check(viewGuardMsg(r), "result mentions not allowed/not a function (got: " + r.result + ")");
    }
    // ============================================================
    // [t4] view guard: get_granule(path, ttl) → "operation not allowed in view"
    // ============================================================
    {
        console.log("\n[t4] t_view_create (view) → not allowed");
        const r = await mtjs.call_view(node, "storage_test", "t_view_create", {}, 5, 5);
        console.log("  result:", JSON.stringify(r));
        check(r.error === true, "error=true");
        check(viewGuardMsg(r), "result mentions not allowed/not a function (got: " + r.result + ")");
    }
    // ============================================================
    // [t5] view guard: del → not a function
    // ============================================================
    {
        console.log("\n[t5] t_view_del (view) → not allowed / not a function");
        const r = await mtjs.call_view(node, "storage_test", "t_view_del", {}, 5, 5);
        console.log("  result:", JSON.stringify(r));
        check(r.error === true, "error=true");
        check(viewGuardMsg(r), "result mentions not allowed/not a function (got: " + r.result + ")");
    }
    // ============================================================
    // [t6] view guard: callMethod → throw
    // ============================================================
    {
        console.log("\n[t6] t_view_callmethod (view) → not allowed");
        const r = await mtjs.call_view(node, "storage_test", "t_view_callmethod", {}, 5, 5);
        console.log("  result:", JSON.stringify(r));
        check(r.error === true, "error=true");
        check(viewGuardMsg(r), "result mentions not allowed (got: " + r.result + ")");
    }
    // ============================================================
    // [t7] view guard: callView → throw
    // ============================================================
    {
        console.log("\n[t7] t_view_callview (view) → not allowed");
        const r = await mtjs.call_view(node, "storage_test", "t_view_callview", {}, 5, 5);
        console.log("  result:", JSON.stringify(r));
        check(r.error === true, "error=true");
        check(viewGuardMsg(r), "result mentions not allowed (got: " + r.result + ")");
    }
    // ============================================================
    // [t8] method: del → get_contract_data = false
    // ============================================================
    {
        console.log("\n[t8] t_simple_del (method) — del place11");
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "storage_test", method: "t_simple_del", params: {} },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "t_simple_del ok (errstr=" + (block.errstr(rsp.tx_hash) || "") + ")");
        const r = await mtjs.get_contract_data(node, "storage_test", "concert/sectorA/place11", 3);
        check(r.found === false, "place11 deleted (found=false)");
    }
    // ============================================================
    // [t9] GC: t_short_ttl (ttl=2) → scan → gc → not found
    // ============================================================
    {
        console.log("\n[t9] t_short_ttl (ttl=2) → scan → gc → not found");
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "storage_test", method: "t_short_ttl", params: {} },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "t_short_ttl ok");
        const g0 = await mtjs.get_contract_data(node, "storage_test", "short", 3);
        check(g0.found === true, "short found (before GC)");
        await new Promise(r => setTimeout(r, 3000));
        let applied = 0;
        for (let i = 0; i < 5; i++) {
            const r = await mtjs.scan_expired(node, "", 5);
            if (r.dbids.length > 0) {
                const { rsp: grsp, block: gb } = await submitAndWait(rootSk, rootAddr, [
                    { contract: "root", method: "gc", params: { items: r.dbids } },
                ]);
                check(gb.status(grsp.tx_hash) === "ok", "gc #" + (i + 1) + " ok");
                const em = gb.children(grsp.tx_hash);
                if (em)
                    for (const k of Object.keys(em)) {
                        for (const e of em[k].emits) {
                            if (e[0] === "gc") {
                                let v = e[1];
                                if (typeof v === "string")
                                    try {
                                        v = JSON.parse(v);
                                    }
                                    catch (_) { }
                                if (v && typeof v.applied === "number")
                                    applied += v.applied;
                            }
                        }
                    }
            }
            if (r.done)
                break;
        }
        check(applied > 0, "gc applied > 0 (got " + applied + ")");
        const g1 = await mtjs.get_contract_data(node, "storage_test", "short", 3);
        check(g1.found === false, "short deleted (found=false)");
        const c = await mtjs.get_contract(node, "storage_test", 3);
        check(c.found === true, "storage_test contract alive");
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
