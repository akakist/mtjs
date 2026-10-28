#!/usr/local/bin/mtjs
import * as std from "std";

// chk-submit-cb.ts — tx_submit(..., cb) → cb({tx_hash: hex, status: submitted|finalized|timeout|err})
// node :2301.

const node = "127.0.0.1:2301";

const rootSk = std.getenv("u_root_ed_sk") || std.getenv("k_root_ed_sk");
const rootPk = std.getenv("k_root_ed_pk");
if (!rootSk || !rootPk) throw new Error("no root sk/pk");

const u1sk = std.getenv("k_u1_ed_sk");
const u1pk = std.getenv("k_u1_ed_pk");
if (!u1sk || !u1pk) throw new Error("no k_u1_* keys");

const GREEN = "\x1b[32m"; const RED = "\x1b[31m"; const RESET = "\x1b[0m";
let pass = 0; let fail = 0;
function check(cond: boolean, msg: string) {
    if (cond) { pass++; console.log("  " + GREEN + "✅" + RESET + " " + msg); }
    else      { fail++; console.log("  " + RED   + "❌" + RESET + " " + msg); }
}
function errStr(e: any): string {
    if (e && typeof e === "object" && ("errcode" in e || "errstr" in e))
        return "errcode=" + e.errcode + " errstr=" + e.errstr;
    return String(e);
}

// pending tx → block (для проверки block.status/tx_hashes)
const pending: { [h: string]: { resolve: (b: Block) => void; reject: (e: any) => void; timer: any } } = {};
mtjs.tx_subscribe(node, (b: Block) => {
    for (const h of b.tx_hashes) {
        const p = pending[h];
        if (p) { delete pending[h]; clearTimeout(p.timer); p.resolve(b); }
    }
});
function waitTx(h: string, to: number): Promise<Block> {
    return new Promise<Block>((res, rej) => {
        const timer = setTimeout(() => {
            delete pending[h];
            rej({ errcode: 1, errstr: "timed out (no block with tx " + h.slice(0,8) + ")" });
        }, to * 1000);
        pending[h] = { resolve: res, reject: rej, timer };
    });
}

async function getNonce(addr: string): Promise<bigint> {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    return BigInt(ui.nonce);
}
async function waitNonceGt(addr: string, lastNonce: bigint, timeoutSec = 10): Promise<bigint> {
    const t0 = Date.now();
    while (Date.now() - t0 < timeoutSec * 1000) {
        const n = await getNonce(addr);
        if (n > lastNonce) return n;
        await new Promise<void>((r) => setTimeout(() => r(), 200));
    }
    throw new Error("waitNonceGt timeout: lastNonce=" + lastNonce.toString());
}

const rootAddr = mtjs.addr_from_pk(rootPk!);
const u1Addr   = mtjs.addr_from_pk(u1pk!);

// ============================================================
// setup: root mint (value=0), u1 +1000
// ============================================================
async function setup() {
    console.log("\n[setup] fill u1 with 1000");
    // mint root → root (value=0, gasLimit=20000)
    {
        const ui = await mtjs.get_user_nonce(node, rootAddr, 3);
        const req = { tx: [{contract:"root",method:"mint",params:{amount:"5000000"}}],
                      nonce: ui.nonce, value: 0, gasLimit: 20000, gasPrice: 10 };
        const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), rootSk!, ui.nonce, () => {});
        if (rsp.err) throw new Error("setup mint: " + rsp.err_str);
        const b = await waitTx(rsp.tx_hash, 10);
        await waitNonceGt(rootAddr, BigInt(ui.nonce), 10);
        console.log("  mint status:", b.status(rsp.tx_hash));
    }
    // transfer 1000 u1 (value=100000, amount=1000)
    {
        const ui = await mtjs.get_user_nonce(node, rootAddr, 3);
        const req = { tx: [{contract:"root",method:"transfer",params:{to:u1Addr,amount:"1000"}}],
                      nonce: ui.nonce, value: 100000, gasLimit: 20000, gasPrice: 10 };
        const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), rootSk!, ui.nonce, () => {});
        if (rsp.err) throw new Error("setup transfer: " + rsp.err_str);
        const b = await waitTx(rsp.tx_hash, 10);
        await waitNonceGt(rootAddr, BigInt(ui.nonce), 10);
        console.log("  transfer status:", b.status(rsp.tx_hash));
    }
    const ui = await mtjs.get_user_nonce(node, u1Addr, 3);
    check(BigInt(ui.balance) >= 1000n, "u1 funded >= 1000 (balance=" + ui.balance + ")");
}

// ============================================================
// t1: ok TX → cb submitted + finalized
// ============================================================
async function t1_ok() {
    console.log("\n[t1] tx_submit(cb) ok → submitted + finalized");
    const ui = await mtjs.get_user_nonce(node, u1Addr, 3);
    const cbs: any[] = [];
    const tx = [{contract:"root",method:"transfer",params:{to:rootAddr,amount:"100"}}];
    const req = { tx, nonce: ui.nonce, value: 1000000, gasLimit: 100000, gasPrice: 10 };
    const rsp = await mtjs.tx_submit(node, 10, JSON.stringify(req), u1sk!, ui.nonce, (st: any) => {
        cbs.push(st);
        console.log("  cb:", JSON.stringify(st));
    });
    if (rsp.err) { check(false, "submit: " + rsp.err_str); return; }
    const txHash = rsp.tx_hash;
    const b = await waitTx(txHash, 10);
    await waitNonceGt(u1Addr, BigInt(ui.nonce), 10);
    await new Promise<void>((r) => setTimeout(r, 200));

    check(cbs.length >= 1, "cb called at least once (got " + cbs.length + ")");
    const submitted = cbs.find(c => c.status === "submitted");
    const finalized = cbs.find(c => c.status === "finalized");
    check(!!submitted, "cb submitted present");
    check(!!finalized, "cb finalized present");
    if (submitted) {
        check(typeof submitted.tx_hash === "string" && /^[0-9a-f]{64}$/.test(submitted.tx_hash),
              "submitted.tx_hash hex (64)");
        check(submitted.tx_hash === txHash, "submitted.tx_hash == rsp.tx_hash");
    }
    if (finalized) {
        check(typeof finalized.tx_hash === "string" && /^[0-9a-f]{64}$/.test(finalized.tx_hash),
              "finalized.tx_hash hex (64)");
        check(finalized.tx_hash === txHash, "finalized.tx_hash == rsp.tx_hash");
    }
    check(b.status(txHash) === "ok", "block.status == ok");
}

// ============================================================
// t2: err TX (amount>t.value) → submitted + finalized (block err)
// ============================================================
async function t2_err() {
    console.log("\n[t2] tx_submit(cb) err (amount>t.value)");
    const ui = await mtjs.get_user_nonce(node, u1Addr, 3);
    const cbs: any[] = [];
    const tx = [{contract:"root",method:"transfer",params:{to:rootAddr,amount:"2000000"}}];
    const req = { tx, nonce: ui.nonce, value: 1000000, gasLimit: 100000, gasPrice: 10 };
    const rsp = await mtjs.tx_submit(node, 10, JSON.stringify(req), u1sk!, ui.nonce, (st: any) => {
        cbs.push(st);
        console.log("  cb:", JSON.stringify(st));
    });
    if (rsp.err) { check(false, "submit: " + rsp.err_str); return; }
    const txHash = rsp.tx_hash;
    const b = await waitTx(txHash, 10);
    await waitNonceGt(u1Addr, BigInt(ui.nonce), 10);
    await new Promise<void>((r) => setTimeout(r, 200));

    check(cbs.length >= 1, "cb called at least once (got " + cbs.length + ")");
    const submitted = cbs.find(c => c.status === "submitted");
    const finalized = cbs.find(c => c.status === "finalized");
    // amount>t.value — TX принята в пул (AddTxRSP errcode=0), блок err → submitted + finalized
    check(!!submitted, "cb submitted present (errcode=0, block err)");
    check(!!finalized, "cb finalized present (block err)");
    // block.status == err (cb не получает block, status только в block)
    check(b.status(txHash) === "err", "block.status == err");
    // cb НЕ должен слать {status:"err"} для block err (это AddTxRSP errcode, не block)
    const cbErr = cbs.find(c => c.status === "err");
    check(!cbErr, "cb status=err NOT present (block err не AddTxRSP err)");
}

// ============================================================
// t3: timeout (timeout=0.001) → cb timeout
// ============================================================
async function t3_timeout() {
    console.log("\n[t3] tx_submit(cb, timeout=0.001) → cb timeout");
    const ui = await mtjs.get_user_nonce(node, u1Addr, 3);
    const cbs: any[] = [];
    const tx = [{contract:"root",method:"transfer",params:{to:rootAddr,amount:"100"}}];
    const req = { tx, nonce: ui.nonce, value: 1000000, gasLimit: 100000, gasPrice: 10 };
    const rsp = await mtjs.tx_submit(node, 0.001, JSON.stringify(req), u1sk!, ui.nonce, (st: any) => {
        cbs.push(st);
        console.log("  cb:", JSON.stringify(st));
    });
    if (rsp.err) { check(false, "submit: " + rsp.err_str); return; }
    // ждём timeout (0.001s) + запас
    await new Promise<void>((r) => setTimeout(r, 1000));
    const timeout = cbs.find(c => c.status === "timeout");
    check(!!timeout, "cb timeout present (got " + cbs.length + " cb)");
    if (timeout) {
        check(typeof timeout.tx_hash === "string" && /^[0-9a-f]{64}$/.test(timeout.tx_hash),
              "timeout.tx_hash hex (64)");
    }
    // TX могла попасть в блок после timeout — не проверяем (гонка)
}

// ============================================================
// t4: 2 TX в одном блоке → 2 cb (каждый свой hex)
// ============================================================
async function t4_two_tx() {
    console.log("\n[t4] 2 TX в одном блоке → 2 cb");
    const ui = await mtjs.get_user_nonce(node, u1Addr, 3);
    const n0 = BigInt(ui.nonce);
    const cbs: { [h: string]: any[] } = {};
    const hashes: string[] = [];

    // два tx_submit с nonce n0, n0+1 (в один блок, если успеют)
    for (let i = 0; i < 2; i++) {
        const tx = [{contract:"root",method:"transfer",params:{to:rootAddr,amount:"10"}}];
        const req = { tx, nonce: (n0 + BigInt(i)).toString(), value: 1000000, gasLimit: 100000, gasPrice: 10 };
        const rsp = await mtjs.tx_submit(node, 10, JSON.stringify(req), u1sk!, (n0 + BigInt(i)).toString(), (st: any) => {
            if (!cbs[st.tx_hash]) cbs[st.tx_hash] = [];
            cbs[st.tx_hash].push(st);
            console.log("  cb[" + st.tx_hash.slice(0,8) + "]:", JSON.stringify(st));
        });
        if (rsp.err) { check(false, "submit " + i + ": " + rsp.err_str); return; }
        hashes.push(rsp.tx_hash);
    }
    // ждём блоки
    for (const h of hashes) await waitTx(h, 10);
    await waitNonceGt(u1Addr, n0 + 1n, 10);
    await new Promise<void>((r) => setTimeout(r, 300));

    check(hashes[0] !== hashes[1], "2 different tx_hash");
    for (const h of hashes) {
        const list = cbs[h] || [];
        check(list.length >= 1, "cb for " + h.slice(0,8) + " present (" + list.length + ")");
        const submitted = list.find(c => c.status === "submitted");
        const finalized = list.find(c => c.status === "finalized");
        check(!!submitted, "  submitted for " + h.slice(0,8));
        check(!!finalized, "  finalized for " + h.slice(0,8));
    }
}

// ============================================================
// main
// ============================================================
(async () => {
    console.log("=== SUBMIT CB TESTS ===");
    await setup();
    await t1_ok();
    await t2_err();
    await t3_timeout();
    await t4_two_tx();
    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0) console.log(GREEN + "DONE" + RESET);
    else            console.log(RED   + "FAIL" + RESET);
})().catch((e: any) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => {}, 1000000);
export {};

