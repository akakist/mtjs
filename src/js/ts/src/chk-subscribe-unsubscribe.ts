#!/usr/local/bin/mtjs
import * as std from "std";

// chk-subscribe-unsubscribe.ts — tx_subscribe / tx_unsubscribe
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

// pending (tx_wait через tx_subscribe callback)
const pending: { [h: string]: { resolve: (b: Block) => void; reject: (e: any) => void; timer: any } } = {};
let subBlocks = 0;
mtjs.tx_subscribe(node, (b: Block) => {
    subBlocks++;
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

// ok TX (transfer 100 от u1), value=1000000
async function submitTransfer(sk: string, addr: string, to: string, amount: string) {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    const tx = [{contract:"root",method:"transfer",params:{to,amount}}];
    const req = { tx, nonce: ui.nonce, value: 1000000, gasLimit: 100000, gasPrice: 10 };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, ui.nonce, () => {});
    if (rsp.err) throw new Error("submit: " + rsp.err_str);
    const b = await waitTx(rsp.tx_hash, 10);
    await waitNonceGt(addr, BigInt(ui.nonce), 10);
    return { rsp, block: b };
}

// ============================================================
// t1: tx_subscribe работает (блоки приходят)
// ============================================================
async function t1_subscribe() {
    console.log("\n[t1] tx_subscribe → блоки приходят");
    const before = subBlocks;
    await submitTransfer(u1sk!, u1Addr, rootAddr, "100");
    await new Promise<void>((r) => setTimeout(r, 300));
    check(subBlocks > before, "tx_subscribe callback fired (before=" + before + " after=" + subBlocks + ")");
}

// ============================================================
// t2: tx_unsubscribe → блоки не приходят
// ============================================================
async function t2_unsubscribe() {
    console.log("\n[t2] tx_unsubscribe → блоки не приходят");
    mtjs.tx_unsubscribe(node);
    await new Promise<void>((r) => setTimeout(r, 300));
    const before = subBlocks;
    // прямой tx_submit, без waitTx (tx_subscribe отписан → pending не резолвится)
    const ui = await mtjs.get_user_nonce(node, u1Addr, 3);
    const tx = [{contract:"root",method:"transfer",params:{to:rootAddr,amount:"100"}}];
    const req = { tx, nonce: ui.nonce, value: 1000000, gasLimit: 100000, gasPrice: 10 };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), u1sk!, ui.nonce, () => {});
    if (rsp.err) { check(false, "submit: " + rsp.err_str); return; }
    await waitNonceGt(u1Addr, BigInt(ui.nonce), 10);  // nonce inc — блок собран, но subBlocks не растёт
    await new Promise<void>((r) => setTimeout(r, 500));
    check(subBlocks === before, "tx_subscribe callback NOT fired after unsubscribe (before=" + before + " after=" + subBlocks + ")");

    // снова подписаться (для t3/t4)
    mtjs.tx_subscribe(node, (b: Block) => {
        subBlocks++;
        for (const h of b.tx_hashes) {
            const p = pending[h];
            if (p) { delete pending[h]; clearTimeout(p.timer); p.resolve(b); }
        }
    });
    await new Promise<void>((r) => setTimeout(r, 300));
}

// ============================================================
// t3: tx_subscribe + tx_submit(cb) — оба работают
// ============================================================
async function t3_both() {
    console.log("\n[t3] tx_subscribe + tx_submit(cb) — оба работают");
    const beforeSub = subBlocks;
    const ui = await mtjs.get_user_nonce(node, u1Addr, 3);
    const cbs: any[] = [];
    const tx = [{contract:"root",method:"transfer",params:{to:rootAddr,amount:"100"}}];
    const req = { tx, nonce: ui.nonce, value: 1000000, gasLimit: 100000, gasPrice: 10 };
    const rsp = await mtjs.tx_submit(node, 10, JSON.stringify(req), u1sk!, ui.nonce, (st: any) => {
        cbs.push(st);
        console.log("  cb:", JSON.stringify(st));
    });
    if (rsp.err) { check(false, "submit: " + rsp.err_str); return; }
    await waitTx(rsp.tx_hash, 10);
    await waitNonceGt(u1Addr, BigInt(ui.nonce), 10);
    await new Promise<void>((r) => setTimeout(r, 300));

    check(subBlocks > beforeSub, "tx_subscribe fired (before=" + beforeSub + " after=" + subBlocks + ")");
    const submitted = cbs.find(c => c.status === "submitted");
    const finalized = cbs.find(c => c.status === "finalized");
    check(!!submitted, "cb submitted present");
    check(!!finalized, "cb finalized present");
}

// ============================================================
// t4: tx_unsubscribe не убивает tx_submit(cb)
// ============================================================
async function t4_unsub_not_kill_cb() {
    console.log("\n[t4] tx_unsubscribe не убивает tx_submit(cb)");
    mtjs.tx_unsubscribe(node);
    await new Promise<void>((r) => setTimeout(r, 300));

    const ui = await mtjs.get_user_nonce(node, u1Addr, 3);
    const cbs: any[] = [];
    const tx = [{contract:"root",method:"transfer",params:{to:rootAddr,amount:"100"}}];
    const req = { tx, nonce: ui.nonce, value: 1000000, gasLimit: 100000, gasPrice: 10 };
    const rsp = await mtjs.tx_submit(node, 10, JSON.stringify(req), u1sk!, ui.nonce, (st: any) => {
        cbs.push(st);
        console.log("  cb:", JSON.stringify(st));
    });
    if (rsp.err) { check(false, "submit: " + rsp.err_str); return; }

    // tx_subscribe отписан, pending не резолвится → ждём finalized через cb (poll)
    const t0 = Date.now();
    while (Date.now() - t0 < 10000) {
        if (cbs.find(c => c.status === "finalized")) break;
        await new Promise<void>((r) => setTimeout(r, 200));
    }
    await waitNonceGt(u1Addr, BigInt(ui.nonce), 10);

    const submitted = cbs.find(c => c.status === "submitted");
    const finalized = cbs.find(c => c.status === "finalized");
    check(!!submitted, "cb submitted present (tx_subscribe отписан, cb работает)");
    check(!!finalized, "cb finalized present (tx_subscribe отписан, cb работает)");
}

// ============================================================
// main
// ============================================================
(async () => {
    console.log("=== SUBSCRIBE/UNSUBSCRIBE TESTS ===");
    await setup();
    await t1_subscribe();
    await t2_unsubscribe();
    await t3_both();
    await t4_unsub_not_kill_cb();
    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0) console.log(GREEN + "DONE" + RESET);
    else            console.log(RED   + "FAIL" + RESET);
})().catch((e: any) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => {}, 1000000);
export {};
