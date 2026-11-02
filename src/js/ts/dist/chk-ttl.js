#!/usr/local/bin/mtjs
import * as std from "std";
const node = "127.0.0.1:2301";
const SRC = "/home/bsr/mtjs/src/js/tsi/dist/contracts/ttl_test.js";
const rootSk = std.getenv("u_root_ed_sk") || std.getenv("k_root_ed_sk");
const rootPk = std.getenv("k_root_ed_pk");
if (!rootSk || !rootPk)
    throw new Error("no root sk/pk");
const GREEN = "\x1b[32m";
const RED = "\x1b[31m";
const RESET = "\x1b[0m";
let pass = 0, fail = 0;
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
    return BigInt((await mtjs.get_user_nonce(node, addr, 3)).nonce);
}
async function waitNonceGt(addr, last, to = 10) {
    const t0 = Date.now();
    while (Date.now() - t0 < to * 1000) {
        const n = await getNonce(addr);
        if (n > last)
            return n;
        await new Promise(r => setTimeout(() => r(), 200));
    }
    throw new Error("waitNonceGt timeout");
}
async function submitAndWait(sk, addr, tx, value = 0, gasLimit = 100000) {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    const req = { tx, nonce: ui.nonce, value, gasLimit, priority: 0 };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, ui.nonce, () => { });
    if (rsp.err)
        throw new Error("submit: " + rsp.err_str);
    const b = await waitTx(rsp.tx_hash, 10);
    await waitNonceGt(addr, BigInt(ui.nonce), 10);
    return { rsp, block: b };
}
async function submitAndWaitErr(sk, addr, tx, value = 0, gasLimit = 100000) {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    const req = { tx, nonce: ui.nonce, value, gasLimit, priority: 0 };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, ui.nonce, () => { });
    if (rsp.err)
        throw new Error("submit: " + rsp.err_str);
    const b = await waitTx(rsp.tx_hash, 10);
    await waitNonceGt(addr, BigInt(ui.nonce), 10);
    return { rsp, block: b };
}
const rootAddr = mtjs.addr_from_pk(rootPk);
function txErr(b, h) {
    return b.errstr(h) || "";
}
// cleanup: удалить гранулу, если есть (от прошлого прогона)
async function cleanupGranule(method, key) {
    const g = await mtjs.get_contract_data(node, "ttl_test", key, 3);
    if (g.found) {
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "ttl_test", method: method, params: {} },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "cleanup " + method + " (errstr=" + txErr(block, rsp.tx_hash) + ")");
    }
}
async function main() {
    console.log("=== TTL TEST (node " + node + ") ===");
    const rb = BigInt((await mtjs.get_user_nonce(node, rootAddr, 3)).balance);
    if (rb < 50000000n) {
        if (rb < 200000n) {
            check(false, "root low — clean db");
            return;
        }
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "root", method: "mint", params: { amount: "100000000" } },
        ], 0, 20000);
        check(block.status(rsp.tx_hash) === "ok", "mint root");
    }
    let src = "";
    try {
        src = await mtjs.fs.readFile(SRC);
    }
    catch (e) {
        check(false, "read src: " + errStr(e));
        return;
    }
    console.log("  src len:", src.length);
    check(src.length > 0, "src read (" + src.length + " bytes)");
    const c = await mtjs.get_contract(node, "ttl_test", 3);
    if (c.found) {
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "root", method: "contract_update", params: { name: "ttl_test", src } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "contract_update (errstr=" + txErr(block, rsp.tx_hash) + ")");
    }
    else {
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "root", method: "contract_deploy", params: { name: "ttl_test", src } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "contract_deploy (errstr=" + txErr(block, rsp.tx_hash) + ")");
    }
    // cleanup x (от прошлого прогона)
    await cleanupGranule("deleteX", "x");
    // t1: createGranule с ttl=86400 (1 день) — ok
    {
        console.log("\n[t1] createGranule ttl=86400 (1 день) — ok");
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "ttl_test", method: "createX", params: { ttl: 86400, v: 1 } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "createX ok (errstr=" + txErr(block, rsp.tx_hash) + ")");
        const g = await mtjs.get_contract_data(node, "ttl_test", "x", 3);
        check(g.found === true, "x found");
    }
    // t2: createGranule ttl=0 — err (ttl > 0)
    {
        console.log("\n[t2] createGranule ttl=0 — err (ttl > 0)");
        const { rsp, block } = await submitAndWaitErr(rootSk, rootAddr, [
            { contract: "ttl_test", method: "createY", params: { ttl: 0 } },
        ]);
        check(block.status(rsp.tx_hash) === "err", "createY ttl=0 err");
        const es = txErr(block, rsp.tx_hash);
        check(es.indexOf("ttl must be > 0") >= 0, "errstr: ttl must be > 0 (got: " + es + ")");
    }
    // t3: createGranule ttl > MAX_GRANULE_TTL (10 лет = 315360000) — err
    {
        console.log("\n[t3] createGranule ttl > MAX (10 лет) — err");
        const { rsp, block } = await submitAndWaitErr(rootSk, rootAddr, [
            { contract: "ttl_test", method: "createZ", params: { ttl: 315360001 } },
        ]);
        check(block.status(rsp.tx_hash) === "err", "createZ ttl > max err");
        const es = txErr(block, rsp.tx_hash);
        check(es.indexOf("MAX_TTL") >= 0, "errstr: MAX_TTL (got: " + es + ")");
    }
    // t4: setGranule x (автопродление, старый ttl=86400) — ok
    {
        console.log("\n[t4] setGranule x (автопродление, ttl=86400) — ok");
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "ttl_test", method: "setX", params: { v: 2 } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "setX ok (errstr=" + txErr(block, rsp.tx_hash) + ")");
        const g = await mtjs.get_contract_data(node, "ttl_test", "x", 3);
        check(g.found === true, "x found (после set)");
        if (g.found) {
            const v = JSON.parse(g.value);
            check(v.v === 2, "x.v == 2 (got " + v.v + ")");
        }
    }
    // t5: deleteGranule x — ok
    {
        console.log("\n[t5] deleteGranule x — ok");
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "ttl_test", method: "deleteX", params: {} },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "deleteX ok (errstr=" + txErr(block, rsp.tx_hash) + ")");
        const g = await mtjs.get_contract_data(node, "ttl_test", "x", 3);
        check(g.found === false, "x deleted (found=false)");
    }
    // t6: createGranule без ttl (undefined) — err
    {
        console.log("\n[t6] createGranule без ttl (undefined) — err");
        const { rsp, block } = await submitAndWaitErr(rootSk, rootAddr, [
            { contract: "ttl_test", method: "createY", params: {} }, // ttl отсутствует
        ]);
        check(block.status(rsp.tx_hash) === "err", "createY без ttl err");
        const es = txErr(block, rsp.tx_hash);
        console.log("  errstr:", es);
        check(es.indexOf("ttl") >= 0, "errstr mentions ttl (got: " + es + ")");
    }
    // cleanup y/z (если созданы — не должны)
    // (t2/t3/t6 — err, y/z не созданы; cleanup не нужен)
    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0)
        console.log(GREEN + "DONE" + RESET);
    else
        console.log(RED + "FAIL" + RESET);
}
main().catch((e) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => { }, 1000000);
