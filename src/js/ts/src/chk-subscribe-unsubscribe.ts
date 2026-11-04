#!/usr/local/bin/mtjs
import * as std from "std";

const node = "127.0.0.1:2301";

const KEYS_PATH = std.getenv("MTJS_KEYS_PATH") || "keys_test.json";
let rootSk = ""; let rootPk = "";
let u1Sk = ""; let u1Pk = "";
async function loadKeys() {
    const raw = await mtjs.fs.readFile(KEYS_PATH);
    const keys = JSON.parse(raw);
    const r = keys.users?.root?.keys?.[0];
    if (!r?.sk || !r?.pk) throw new Error("keys_test.json: no users.root.keys[0].sk/pk");
    rootSk = r.sk; rootPk = r.pk;
    const u = keys.users?.u1?.keys?.[0];
    if (!u?.sk || !u?.pk) throw new Error("keys_test.json: no users.u1.keys[0].sk/pk");
    u1Sk = u.sk; u1Pk = u.pk;
}

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
    console.log("getNonce: " + ui.nonce);
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

async function submitAndWait(sk: string, addr: string, tx: any[], value = 0, gasLimit = 20000, retries = 5) {
    for (let attempt = 0; attempt < retries; attempt++) {
        const nonce = await getNonce(addr);
        const req = { tx, nonce: nonce.toString(), value, gasLimit, priority: 0, sender: addr, key_id: 0 };
        const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, nonce.toString(), () => {});
        if (rsp.err) {
            if (attempt + 1 < retries) { await new Promise(r => setTimeout(r, 300)); continue; }
            throw new Error("submit: " + rsp.err_str);
        }
        const b = await waitTx(rsp.tx_hash, 10);
        const st = b.status(rsp.tx_hash);
        const es = b.errstr(rsp.tx_hash) || "";
        if (st === "err" && es.indexOf("invalid nonce") >= 0) {
            if (attempt + 1 < retries) { await new Promise(r => setTimeout(r, 300)); continue; }
            throw new Error("invalid nonce: " + es);
        }
        return { rsp, block: b, nonce };
    }
    throw new Error("submitAndWait: retries exhausted");
}

const rootAddr = mtjs.addr_from_name("root");
const u1Addr   = mtjs.addr_from_name("u1");

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

async function submitTransfer(sk: string, addr: string, to: string, amount: string) {
    return await submitAndWait(sk, addr, [
        { contract: "root", method: "transfer", params: { to, amount } },
    ], 1000000, 100000);
}

async function t1_subscribe() {
    console.log("\n[t1] tx_subscribe → блоки приходят");
    const before = subBlocks;
    await submitTransfer(u1Sk, u1Addr, rootAddr, "100");
    await new Promise<void>((r) => setTimeout(r, 300));
    check(subBlocks > before, "tx_subscribe callback fired (before=" + before + " after=" + subBlocks + ")");
}

async function t2_unsubscribe() {
    console.log("\n[t2] tx_unsubscribe → блоки не приходят");
    mtjs.tx_unsubscribe(node);
    await new Promise<void>((r) => setTimeout(r, 300));
    const before = subBlocks;

    await submitAndWait(u1Sk, u1Addr, [
        { contract: "root", method: "transfer", params: { to: rootAddr, amount: "100" } },
    ], 1000000, 100000);

    await new Promise<void>((r) => setTimeout(r, 500));
    check(subBlocks === before, "tx_subscribe callback NOT fired after unsubscribe (before=" + before + " after=" + subBlocks + ")");

    mtjs.tx_subscribe(node, (b: Block) => {
        subBlocks++;
        for (const h of b.tx_hashes) {
            const p = pending[h];
            if (p) { delete pending[h]; clearTimeout(p.timer); p.resolve(b); }
        }
    });
    await new Promise<void>((r) => setTimeout(r, 300));
}

async function t3_both() {
    console.log("\n[t3] tx_subscribe + tx_submit(cb) — оба работают");
    const beforeSub = subBlocks;
    const ui = await mtjs.get_user_nonce(node, u1Addr, 3);
    const cbs: any[] = [];
    const tx = [{contract:"root",method:"transfer",params:{to:rootAddr,amount:"100"}}];
    const req = { tx, nonce: ui.nonce, value: 1000000, gasLimit: 100000, priority: 0, sender: u1Addr, key_id: 0 };
    const rsp = await mtjs.tx_submit(node, 10, JSON.stringify(req), u1Sk, ui.nonce, (st: any) => {
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

async function t4_unsub_not_kill_cb() {
    console.log("\n[t4] tx_unsubscribe не убивает tx_submit(cb)");
    mtjs.tx_unsubscribe(node);
    await new Promise<void>((r) => setTimeout(r, 300));

    const ui = await mtjs.get_user_nonce(node, u1Addr, 3);
    const cbs: any[] = [];
    const tx = [{contract:"root",method:"transfer",params:{to:rootAddr,amount:"100"}}];
    const req = { tx, nonce: ui.nonce, value: 1000000, gasLimit: 100000, priority: 0, sender: u1Addr, key_id: 0 };
    const rsp = await mtjs.tx_submit(node, 10, JSON.stringify(req), u1Sk, ui.nonce, (st: any) => {
        cbs.push(st);
        console.log("  cb:", JSON.stringify(st));
    });
    if (rsp.err) { check(false, "submit: " + rsp.err_str); return; }

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

(async () => {
    await loadKeys();
    console.log("=== SUBSCRIBE/UNSUBSCRIBE TESTS ===");
    await setup();
    await t1_subscribe();
    await t2_unsubscribe();
    await t3_both();
    await t4_unsub_not_kill_cb();
    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0) console.log(GREEN + "DONE" + RESET);
    else            console.log(RED   + "FAIL" + RESET);
    std.exit(fail);
})().catch((e: any) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => {}, 1000000);
export {};