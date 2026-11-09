#!/usr/local/bin/mtjs
import * as std from "std";
// chk-submit-cb.ts — tx_submit(..., cb) → cb({tx_hash: hex, status: submitted|finalized|timeout|err})
// node :2301.
const node = "127.0.0.1:2301";
const KEYS_PATH = std.getenv("MTJS_KEYS_PATH") || "keys_test.json";
let rootSk = "";
let rootPk = "";
let u1Sk = "";
let u1Pk = "";
async function loadKeys() {
    const raw = await mtjs.fs.readFile(KEYS_PATH);
    const keys = JSON.parse(raw);
    const r = keys.users?.root?.keys?.[0];
    if (!r?.sk || !r?.pk)
        throw new Error("keys_test.json: no users.root.keys[0].sk/pk");
    rootSk = r.sk;
    rootPk = r.pk;
    const u = keys.users?.u1?.keys?.[0];
    if (!u?.sk || !u?.pk)
        throw new Error("keys_test.json: no users.u1.keys[0].sk/pk");
    u1Sk = u.sk;
    u1Pk = u.pk;
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
    if (e && typeof e === "object" && ("errcode" in e || "errstr" in e))
        return "errcode=" + e.errcode + " errstr=" + e.errstr;
    return String(e);
}
const pending = {};
let lastBlock = null; // кэш последнего блока
mtjs.tx_subscribe(node, (b) => {
    lastBlock = b; // сохраняем последний блок
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
    // если блок уже пришёл (lastBlock) и содержит tx — resolve сразу
    if (lastBlock && lastBlock.tx_hashes.indexOf(h) >= 0) {
        return Promise.resolve(lastBlock);
    }
    return new Promise((res, rej) => {
        const timer = setTimeout(() => {
            delete pending[h];
            rej({ errcode: 1, errstr: "timed out (no block with tx " + h.slice(0, 8) + ")" });
        }, to * 1000);
        pending[h] = { resolve: res, reject: rej, timer };
    });
}
async function getNonce(addr) {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    console.log("getNonce: " + ui.nonce);
    return BigInt(ui.nonce);
}
async function waitNonceGt(addr, lastNonce, timeoutSec = 10) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeoutSec * 1000) {
        const n = await getNonce(addr);
        if (n > lastNonce)
            return n;
        await new Promise((r) => setTimeout(() => r(), 200));
    }
    throw new Error("waitNonceGt timeout: lastNonce=" + lastNonce.toString());
}
async function submitAndWait(sk, addr, tx, value = 0, gasLimit = 20000, retries = 5) {
    for (let attempt = 0; attempt < retries; attempt++) {
        const nonce = await getNonce(addr);
        const req = { tx, nonce: nonce.toString(), value, gasLimit, priority: 0, sender: addr, key_id: 0 };
        const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, nonce.toString(), () => { });
        if (rsp.err) {
            if (attempt + 1 < retries) {
                await new Promise(r => setTimeout(r, 300));
                continue;
            }
            throw new Error("submit: " + rsp.err_str);
        }
        const b = await waitTx(rsp.tx_hash, 10);
        const st = b.status(rsp.tx_hash);
        const es = b.errstr(rsp.tx_hash) || "";
        if (st === "err" && es.indexOf("invalid nonce") >= 0) {
            if (attempt + 1 < retries) {
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
const u1Addr = mtjs.addr_from_name("u1");
async function setup() {
    console.log("\n[setup] fill u1 with 1000");
    {
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "root", method: "mint", params: { amount: "5000000" } },
        ], 0, 20000);
        console.log("  mint status:", block.status(rsp.tx_hash));
    }
    {
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "root", method: "transfer", params: { to: u1Addr, amount: "1000" } },
        ], 100000, 20000);
        console.log("  transfer status:", block.status(rsp.tx_hash));
    }
    const ui = await mtjs.get_user_nonce(node, u1Addr, 3);
    check(BigInt(ui.balance) >= 1000n, "u1 funded >= 1000 (balance=" + ui.balance + ")");
}
async function t1_ok() {
    console.log("\n[t1] tx_submit(cb) ok → submitted + finalized");
    const ui = await mtjs.get_user_nonce(node, u1Addr, 3);
    const cbs = [];
    const tx = [{ contract: "root", method: "transfer", params: { to: rootAddr, amount: "100" } }];
    const req = { tx, nonce: ui.nonce, value: 1000000, gasLimit: 100000, priority: 0, sender: u1Addr, key_id: 0 };
    const rsp = await mtjs.tx_submit(node, 10, JSON.stringify(req), u1Sk, ui.nonce, (st) => {
        cbs.push(st);
        console.log("  cb:", JSON.stringify(st));
    });
    if (rsp.err) {
        check(false, "submit: " + rsp.err_str);
        return;
    }
    const txHash = rsp.tx_hash;
    const b = await waitTx(txHash, 10);
    await waitNonceGt(u1Addr, BigInt(ui.nonce), 10);
    await new Promise((r) => setTimeout(r, 200));
    check(cbs.length >= 1, "cb called at least once (got " + cbs.length + ")");
    const submitted = cbs.find(c => c.status === "submitted");
    const finalized = cbs.find(c => c.status === "finalized");
    check(!!submitted, "cb submitted present");
    check(!!finalized, "cb finalized present");
    if (submitted) {
        check(typeof submitted.tx_hash === "string" && /^[0-9a-f]{64}$/.test(submitted.tx_hash), "submitted.tx_hash hex (64)");
        check(submitted.tx_hash === txHash, "submitted.tx_hash == rsp.tx_hash");
    }
    if (finalized) {
        check(typeof finalized.tx_hash === "string" && /^[0-9a-f]{64}$/.test(finalized.tx_hash), "finalized.tx_hash hex (64)");
        check(finalized.tx_hash === txHash, "finalized.tx_hash == rsp.tx_hash");
    }
    check(b.status(txHash) === "ok", "block.status == ok");
}
async function t2_err() {
    console.log("\n[t2] tx_submit(cb) err (amount>t.value)");
    const ui = await mtjs.get_user_nonce(node, u1Addr, 3);
    const cbs = [];
    const tx = [{ contract: "root", method: "transfer", params: { to: rootAddr, amount: "2000000" } }];
    const req = { tx, nonce: ui.nonce, value: 1000000, gasLimit: 100000, priority: 0, sender: u1Addr, key_id: 0 };
    const rsp = await mtjs.tx_submit(node, 10, JSON.stringify(req), u1Sk, ui.nonce, (st) => {
        cbs.push(st);
        console.log("  cb:", JSON.stringify(st));
    });
    if (rsp.err) {
        check(false, "submit: " + rsp.err_str);
        return;
    }
    const txHash = rsp.tx_hash;
    const b = await waitTx(txHash, 30);
    await waitNonceGt(u1Addr, BigInt(ui.nonce), 10);
    await new Promise((r) => setTimeout(r, 200));
    check(cbs.length >= 1, "cb called at least once (got " + cbs.length + ")");
    const submitted = cbs.find(c => c.status === "submitted");
    const finalized = cbs.find(c => c.status === "finalized");
    check(!!submitted, "cb submitted present (errcode=0, block err)");
    check(!!finalized, "cb finalized present (block err)");
    check(b.status(txHash) === "err", "block.status == err");
    const cbErr = cbs.find(c => c.status === "err");
    check(!cbErr, "cb status=err NOT present (block err не AddTxRSP err)");
}
async function t3_timeout() {
    console.log("\n[t3] tx_submit(cb, timeout=0.001) → cb timeout");
    const ui = await mtjs.get_user_nonce(node, u1Addr, 3);
    const cbs = [];
    const tx = [{ contract: "root", method: "transfer", params: { to: rootAddr, amount: "100" } }];
    const req = { tx, nonce: ui.nonce, value: 1000000, gasLimit: 100000, priority: 0, sender: u1Addr, key_id: 0 };
    const rsp = await mtjs.tx_submit(node, 0.001, JSON.stringify(req), u1Sk, ui.nonce, (st) => {
        cbs.push(st);
        console.log("  cb:", JSON.stringify(st));
    });
    if (rsp.err) {
        check(false, "submit: " + rsp.err_str);
        return;
    }
    await new Promise((r) => setTimeout(r, 1000));
    const timeout = cbs.find(c => c.status === "timeout");
    check(!!timeout, "cb timeout present (got " + cbs.length + " cb)");
    if (timeout) {
        check(typeof timeout.tx_hash === "string" && /^[0-9a-f]{64}$/.test(timeout.tx_hash), "timeout.tx_hash hex (64)");
    }
}
async function t4_two_tx() {
    console.log("\n[t4] 2 TX в одном блоке → 2 cb");
    const ui = await mtjs.get_user_nonce(node, u1Addr, 3);
    const n0 = BigInt(ui.nonce);
    const cbs = {};
    const hashes = [];
    for (let i = 0; i < 2; i++) {
        const tx = [{ contract: "root", method: "transfer", params: { to: rootAddr, amount: "10" } }];
        const req = { tx, nonce: (n0 + BigInt(i)).toString(), value: 1000000, gasLimit: 100000, priority: 0, sender: u1Addr, key_id: 0 };
        const rsp = await mtjs.tx_submit(node, 10, JSON.stringify(req), u1Sk, (n0 + BigInt(i)).toString(), (st) => {
            if (!cbs[st.tx_hash])
                cbs[st.tx_hash] = [];
            cbs[st.tx_hash].push(st);
            console.log("  cb[" + st.tx_hash.slice(0, 8) + "]:", JSON.stringify(st));
        });
        if (rsp.err) {
            check(false, "submit " + i + ": " + rsp.err_str);
            return;
        }
        hashes.push(rsp.tx_hash);
    }
    for (const h of hashes)
        await waitTx(h, 10);
    await waitNonceGt(u1Addr, n0 + 1n, 10);
    await new Promise((r) => setTimeout(r, 300));
    check(hashes[0] !== hashes[1], "2 different tx_hash");
    for (const h of hashes) {
        const list = cbs[h] || [];
        check(list.length >= 1, "cb for " + h.slice(0, 8) + " present (" + list.length + ")");
        const submitted = list.find(c => c.status === "submitted");
        const finalized = list.find(c => c.status === "finalized");
        check(!!submitted, "  submitted for " + h.slice(0, 8));
        check(!!finalized, "  finalized for " + h.slice(0, 8));
    }
}
(async () => {
    await loadKeys();
    console.log("=== SUBMIT CB TESTS ===");
    await setup();
    await t1_ok();
    await t2_err();
    await t3_timeout();
    await t4_two_tx();
    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0)
        console.log(GREEN + "DONE" + RESET);
    else
        console.log(RED + "FAIL" + RESET);
    std.exit(fail);
})().catch((e) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => { }, 1000000);
