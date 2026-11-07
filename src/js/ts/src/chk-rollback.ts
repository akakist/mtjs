#!/usr/local/bin/mtjs
import * as std from "std";

// chk-rollback.ts — on-chain роллбэки.
// node :2301.

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
    console.log("getNonce: " + ui.nonce);
    return BigInt(ui.nonce);
}
async function getNonceBal(addr: string): Promise<{ nonce: bigint; bal: bigint }> {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    return { nonce: BigInt(ui.nonce), bal: BigInt(ui.balance) };
}

async function submitAndWait(sk: string, addr: string, tx: any[], value = 0, gasLimit = 100000, retries = 5) {
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

async function submitAndWaitErr(sk: string, addr: string, tx: any[], value = 0, gasLimit = 100000, retries = 5) {
    for (let attempt = 0; attempt < retries; attempt++) {
        const nonce = await getNonce(addr);
        const req = { tx, nonce: nonce.toString(), value, gasLimit, priority: 0, sender: addr, key_id: 0 };
        const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, nonce.toString(), () => {});
        if (rsp.err) {
            if (attempt + 1 < retries) { await new Promise(r => setTimeout(r, 300)); continue; }
            throw new Error("submit: " + rsp.err_str);
        }
        let b: Block;
        try {
            b = await waitTx(rsp.tx_hash, 30);
        } catch (e) {
            if (attempt + 1 < retries) { await new Promise(r => setTimeout(r, 300)); continue; }
            throw e;
        }
        const st = b.status(rsp.tx_hash);
        const es = b.errstr(rsp.tx_hash) || "";
        if (st === "err" && es.indexOf("invalid nonce") >= 0) {
            if (attempt + 1 < retries) { await new Promise(r => setTimeout(r, 300)); continue; }
            throw new Error("invalid nonce: " + es);
        }
        return { rsp, block: b, nonce };
    }
    throw new Error("submitAndWaitErr: retries exhausted");
}

const rootAddr = mtjs.addr_from_name("root");
const u1Addr   = mtjs.addr_from_name("u1");

async function mintFromRoot(amount: string) {
    return await submitAndWait(rootSk, rootAddr, [
        { contract: "root", method: "mint", params: { amount } },
    ], 0, 20000);
}
async function voteFromRoot(vnode: string, param: string, value: string, ttl: string) {
    return await submitAndWait(rootSk, rootAddr, [
        { contract: "root", method: "vote", params: { node: vnode, param, value, ttl } },
    ], 0, 20000);
}

async function setup() {
    console.log("\n[setup] top up root, fill u1 with 1000");
    {
        const { bal: rb } = await getNonceBal(rootAddr);
        console.log("  root balance:", rb.toString());
        if (rb < 5000000n) {
            console.log("  root balance low, mint 5000000 (root -> root)");
            const { rsp, block } = await mintFromRoot("5000000");
            const st = block.status(rsp.tx_hash);
            console.log("  mint status:", st, "errstr:", block.errstr(rsp.tx_hash));
            if (st === "ok") {
                const { bal: rb2 } = await getNonceBal(rootAddr);
                console.log("  root balance after mint:", rb2.toString());
                check(rb2 > rb, "root topped up: " + rb.toString() + " -> " + rb2.toString());
            } else {
                check(false, "root mint err: " + block.errstr(rsp.tx_hash));
            }
        }
    }
    await submitAndWait(rootSk, rootAddr, [{ contract: "root", method: "mint", params: { amount: "1000" } }]);
    await submitAndWait(rootSk, rootAddr, [{ contract: "root", method: "transfer", params: { to: u1Addr, amount: "1000" } }], 1000);
    const { bal } = await getNonceBal(u1Addr);
    console.log("  u1 balance:", bal.toString());
    check(bal >= 1000n, "u1 funded >= 1000");
}

async function t1_err_state() {
    console.log("\n[t1] err TX: state rolled back, nonce inc, gas charged");
    const { nonce: n0, bal: b0 } = await getNonceBal(u1Addr);
    const { rsp, block } = await submitAndWaitErr(u1Sk, u1Addr, [
        { contract: "root", method: "transfer", params: { to: rootAddr, amount: "2000000" } },
    ]);
    check(block.status(rsp.tx_hash) === "err", "status err");
    const es = block.errstr(rsp.tx_hash);
    console.log("  errstr:", es);
    check(typeof es === "string" && (es.indexOf("amount") >= 0 || es.indexOf("value") >= 0),
          "errstr mentions amount/value");
    const { nonce: n1, bal: b1 } = await getNonceBal(u1Addr);
    check(n1 === n0 + 1n, "nonce inc by 1 (Ethereum): " + n0.toString() + " -> " + n1.toString());
    check(b1 <= b0, "balance not increased (gas charged): " + b0.toString() + " -> " + b1.toString());
}

async function t2_partial_batch() {
    console.log("\n[t2] batch: ok + err (per-TX rollback, gas charged)");
    const { nonce: n0, bal: b0 } = await getNonceBal(u1Addr);
    const { rsp, block } = await submitAndWaitErr(u1Sk, u1Addr, [
        { contract: "root", method: "transfer", params: { to: rootAddr, amount: "100" } },
        { contract: "root", method: "transfer", params: { to: rootAddr, amount: "2000000" } },
    ]);
    const st = block.status(rsp.tx_hash);
    console.log("  batch status:", st, "errstr:", block.errstr(rsp.tx_hash));
    check(st === "err", "batch status err (per-TX)");
    const { nonce: n1, bal: b1 } = await getNonceBal(u1Addr);
    check(n1 === n0 + 1n, "nonce inc by 1: " + n0.toString() + " -> " + n1.toString());
    check(b1 <= b0, "balance not increased (state rolled back, gas charged): " + b0.toString() + " -> " + b1.toString());
}

async function t3_separate_txs() {
    console.log("\n[t3] separate TXs: ok applied, err rolled back, nonce +2");
    const { nonce: n0, bal: b0 } = await getNonceBal(u1Addr);
    const { rsp: r1, block: b1 } = await submitAndWait(u1Sk, u1Addr, [
        { contract: "root", method: "transfer", params: { to: rootAddr, amount: "100" } },
    ], 100);   // value=100
    check(b1.status(r1.tx_hash) === "ok", "after ok: status ok (errstr=" + b1.errstr(r1.tx_hash) + ")");
    const { nonce: n1, bal: bal1 } = await getNonceBal(u1Addr);
    check(n1 === n0 + 1n, "after ok: nonce +1: " + n0.toString() + " -> " + n1.toString());
    check(bal1 <= b0 - 100n, "after ok: balance <= b0 - 100 (value + gas): " + b0.toString() + " -> " + bal1.toString());
    await submitAndWaitErr(u1Sk, u1Addr, [
        { contract: "root", method: "transfer", params: { to: rootAddr, amount: "2000000" } },
    ]);
    const { nonce: n2, bal: b2 } = await getNonceBal(u1Addr);
    check(n2 === n1 + 1n, "after err: nonce +1: " + n1.toString() + " -> " + n2.toString());
    check(b2 <= bal1, "after err: balance not increased (gas charged): " + bal1.toString() + " -> " + b2.toString());
}

async function t4_vote_ttl_no_effect() {
    console.log("\n[t4] vote ttl_expired: fees unchanged, nonce inc");
    const param = "rb_vote_" + String(Date.now());
    const value = "777";
    await voteFromRoot("n5", param, value, "1");
    const v0 = await mtjs.get_values(node, 3);
    const before = v0.fees[param];
    console.log("  fees[param] before:", before);
    await new Promise<void>((r) => setTimeout(() => r(), 3000));
    const { nonce: rn0 } = await getNonceBal(rootAddr);
    const { rsp, block } = await voteFromRoot("n6", param, value, "1");
    const st = block.status(rsp.tx_hash);
    if (st !== "ok") {
        console.log("  t4 errstr:", block.errstr(rsp.tx_hash));
        console.log("  t4 emits:", JSON.stringify(block.emits(rsp.tx_hash)));
    }
    check(st === "ok", "status ok (TX успешна, vote failed)");
    const em = childEmits(block, rsp.tx_hash);
    let failedReason = null;
    for (const k of Object.keys(em)) for (const e of em[k]) if (e[0] === "vote_failed") failedReason = e[1].reason;
    check(failedReason === "ttl_expired", "vote_failed ttl_expired");
    const v1 = await mtjs.get_values(node, 3);
    check(v1.fees[param] === before, "fees[param] unchanged: " + before + " -> " + v1.fees[param]);
    const { nonce: rn1 } = await getNonceBal(rootAddr);
    check(rn1 === rn0 + 1n, "root nonce inc (vote TX в блоке): " + rn0.toString() + " -> " + rn1.toString());
}

function childEmits(b: Block, txHash: string): { [seq: string]: any[] } {
    const ch = b.children(txHash) as any;
    const out: { [seq: string]: any[] } = {};
    if (ch) for (const k of Object.keys(ch)) out[k] = ch[k].emits;
    return out;
}

(async () => {
    await loadKeys();
    console.log("=== ROLLBACK TESTS ===");
    await setup();
    await t1_err_state();
    await t2_partial_batch();
    await t3_separate_txs();
    await t4_vote_ttl_no_effect();
    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0) console.log(GREEN + "DONE" + RESET);
    else            console.log(RED   + "FAIL" + RESET);
    std.exit(fail);
})().catch((e: any) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => {}, 1000000);
export {};
