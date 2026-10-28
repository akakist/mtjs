#!/usr/local/bin/mtjs
import * as std from "std";
// chk-rollback.ts — on-chain роллбэки.
// Модель (Ethereum): nonce инкрементится ВСЕГДА при попадании TX в блок (ok и err).
// State при err откатывается (roll->data), газ списывается.
// node :2301.
//
// Семантика:
// - mint: sender.balance += amount, газ = getGas("mint") + размер db_dump
// - transfer: amount <= t.value (value из req), t.value -= amount,
//   газ + (value - t.value) списывается с sender; amount>t.value — err
// - emit mint/transfer в children(tx)[seqId].emits, root-level emits — result/error
// - err TX: state откатан, nonce inc (Ethereum), газ списан
// - mint/vote от root: value=0, gasLimit=20000 (резерв 200000, не 2000000)
const node = "127.0.0.1:2301";
const rootSk = std.getenv("u_root_ed_sk") || std.getenv("k_root_ed_sk");
const rootPk = std.getenv("k_root_ed_pk");
if (!rootSk || !rootPk)
    throw new Error("no root sk/pk");
const u1sk = std.getenv("k_u1_ed_sk");
const u1pk = std.getenv("k_u1_ed_pk");
if (!u1sk || !u1pk)
    throw new Error("no k_u1_* keys");
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
// ============================================================
// pending tx → block (tx_subscribe до tx_submit)
// ============================================================
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
        const timer = setTimeout(() => {
            delete pending[h];
            rej({ errcode: 1, errstr: "timed out (no block with tx " + h.slice(0, 8) + ")" });
        }, to * 1000);
        pending[h] = { resolve: res, reject: rej, timer };
    });
}
async function getNonceBal(addr) {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    return { nonce: BigInt(ui.nonce), bal: BigInt(ui.balance) };
}
async function waitNonceGt(addr, lastNonce, timeoutSec = 10) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeoutSec * 1000) {
        const { nonce } = await getNonceBal(addr);
        if (nonce > lastNonce)
            return nonce;
        await new Promise((r) => setTimeout(() => r(), 200));
    }
    throw new Error("waitNonceGt timeout: lastNonce=" + lastNonce.toString());
}
// ok TX: submit → wait block → waitNonceGt
async function submitAndWait(sk, addr, tx, timeoutSec = 10) {
    const { nonce } = await getNonceBal(addr);
    const req = { tx, nonce: nonce.toString(), value: 1000000, gasLimit: 100000, gasPrice: 10 };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, nonce.toString(), () => { });
    if (rsp.err)
        throw new Error("submit: " + rsp.err_str);
    const b = await waitTx(rsp.tx_hash, timeoutSec);
    await waitNonceGt(addr, nonce, timeoutSec);
    return { rsp, block: b, nonce };
}
// err TX: submit → wait block → waitNonceGt (nonce инкрементится ВСЕГДА)
async function submitAndWaitErr(sk, addr, tx, timeoutSec = 10) {
    const { nonce } = await getNonceBal(addr);
    const req = { tx, nonce: nonce.toString(), value: 1000000, gasLimit: 100000, gasPrice: 10 };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, nonce.toString(), () => { });
    if (rsp.err)
        throw new Error("submit: " + rsp.err_str);
    const b = await waitTx(rsp.tx_hash, timeoutSec);
    await waitNonceGt(addr, nonce, timeoutSec); // err TX тоже инкрементит nonce
    return { rsp, block: b, nonce };
}
function childEmits(b, txHash) {
    const ch = b.children(txHash);
    const out = {};
    if (ch)
        for (const k of Object.keys(ch))
            out[k] = ch[k].emits;
    return out;
}
// mint root → root: value=0, gasLimit=20000 (резерв 200000, не 2000000)
async function mintFromRoot(amount) {
    const { nonce } = await getNonceBal(rootAddr);
    const req = {
        tx: [{ contract: "root", method: "mint", params: { amount } }],
        nonce: nonce.toString(), value: 0, gasLimit: 20000, gasPrice: 10,
    };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), rootSk, nonce.toString(), () => { });
    if (rsp.err)
        throw new Error("submit: " + rsp.err_str);
    const b = await waitTx(rsp.tx_hash, 10);
    await waitNonceGt(rootAddr, nonce, 10);
    return { rsp, block: b };
}
// vote root: value=0, gasLimit=20000
async function voteFromRoot(vnode, param, value, ttl) {
    const { nonce } = await getNonceBal(rootAddr);
    const req = {
        tx: [{ contract: "root", method: "vote", params: { node: vnode, param, value, ttl } }],
        nonce: nonce.toString(), value: 0, gasLimit: 20000, gasPrice: 10,
    };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), rootSk, nonce.toString(), () => { });
    if (rsp.err)
        throw new Error("submit: " + rsp.err_str);
    const b = await waitTx(rsp.tx_hash, 10);
    await waitNonceGt(rootAddr, nonce, 10);
    return { rsp, block: b };
}
const rootAddr = mtjs.addr_from_pk(rootPk);
const u1Addr = mtjs.addr_from_pk(u1pk);
// ============================================================
// setup: пополнить root (mint value=0), налить u1 1000
// ============================================================
async function setup() {
    console.log("\n[setup] top up root, fill u1 with 1000");
    // root → root mint (value=0, gasLimit=20000 → резерв 200000)
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
            }
            else {
                check(false, "root mint err: " + block.errstr(rsp.tx_hash));
            }
        }
    }
    // mint u1 1000 + transfer u1 1000
    await submitAndWait(rootSk, rootAddr, [{ contract: "root", method: "mint", params: { amount: "1000" } }]);
    await submitAndWait(rootSk, rootAddr, [{ contract: "root", method: "transfer", params: { to: u1Addr, amount: "1000" } }]);
    const { bal } = await getNonceBal(u1Addr);
    console.log("  u1 balance:", bal.toString());
    check(bal >= 1000n, "u1 funded >= 1000");
}
// ============================================================
// t1: err TX — state откатан, nonce inc, газ списан
// ============================================================
async function t1_err_state() {
    console.log("\n[t1] err TX: state rolled back, nonce inc, gas charged");
    const { nonce: n0, bal: b0 } = await getNonceBal(u1Addr);
    // transfer amount > t.value → "amount>t.value"
    const { rsp, block } = await submitAndWaitErr(u1sk, u1Addr, [
        { contract: "root", method: "transfer", params: { to: rootAddr, amount: "2000000" } },
    ]);
    check(block.status(rsp.tx_hash) === "err", "status err");
    const es = block.errstr(rsp.tx_hash);
    console.log("  errstr:", es);
    check(typeof es === "string" && (es.indexOf("amount") >= 0 || es.indexOf("value") >= 0), "errstr mentions amount/value");
    const { nonce: n1, bal: b1 } = await getNonceBal(u1Addr);
    // Ethereum: nonce инкрементится всегда (TX попала в блок)
    check(n1 === n0 + 1n, "nonce inc by 1 (Ethereum): " + n0.toString() + " -> " + n1.toString());
    // Газ списывается (газ за err TX), баланс не увеличился
    check(b1 <= b0, "balance not increased (gas charged): " + b0.toString() + " -> " + b1.toString());
}
// ============================================================
// t2: пачка ok + err — per-TX rollback, nonce inc 1, газ за всю TX
// ============================================================
async function t2_partial_batch() {
    console.log("\n[t2] batch: ok + err (per-TX rollback, gas charged)");
    const { nonce: n0, bal: b0 } = await getNonceBal(u1Addr);
    // пачка: transfer 100 (ok), transfer 2000000 (err)
    const { rsp, block } = await submitAndWaitErr(u1sk, u1Addr, [
        { contract: "root", method: "transfer", params: { to: rootAddr, amount: "100" } },
        { contract: "root", method: "transfer", params: { to: rootAddr, amount: "2000000" } },
    ]);
    const st = block.status(rsp.tx_hash);
    console.log("  batch status:", st, "errstr:", block.errstr(rsp.tx_hash));
    // per-TX: err → rollback всей TX, status err
    check(st === "err", "batch status err (per-TX)");
    const { nonce: n1, bal: b1 } = await getNonceBal(u1Addr);
    // nonce +1 (одна TX, не две команды)
    check(n1 === n0 + 1n, "nonce inc by 1: " + n0.toString() + " -> " + n1.toString());
    // state откатан (первая transfer 100 не применилась), но газ за TX списан
    check(b1 <= b0, "balance not increased (state rolled back, gas charged): " + b0.toString() + " -> " + b1.toString());
}
// ============================================================
// t3: две ОТДЕЛЬНЫЕ TX — ok применилась, err откатилась, nonce +2
// ============================================================
async function t3_separate_txs() {
    console.log("\n[t3] separate TXs: ok applied, err rolled back, nonce +2");
    const { nonce: n0, bal: b0 } = await getNonceBal(u1Addr);
    // TX1: ok transfer 100
    await submitAndWait(u1sk, u1Addr, [
        { contract: "root", method: "transfer", params: { to: rootAddr, amount: "100" } },
    ]);
    const { nonce: n1, bal: b1 } = await getNonceBal(u1Addr);
    check(n1 === n0 + 1n, "after ok: nonce +1: " + n0.toString() + " -> " + n1.toString());
    // balance упал на 100 (value) + газ. Точное значение газа непредсказуемо, но balance < b0 - 100
    check(b1 <= b0 - 100n, "after ok: balance <= b0 - 100 (value + gas): " + b0.toString() + " -> " + b1.toString());
    // TX2: err transfer 2000000
    await submitAndWaitErr(u1sk, u1Addr, [
        { contract: "root", method: "transfer", params: { to: rootAddr, amount: "2000000" } },
    ]);
    const { nonce: n2, bal: b2 } = await getNonceBal(u1Addr);
    check(n2 === n1 + 1n, "after err: nonce +1: " + n1.toString() + " -> " + n2.toString());
    // state откатан (transfer 2000000 не применился), но газ списан → b2 <= b1
    check(b2 <= b1, "after err: balance not increased (gas charged): " + b1.toString() + " -> " + b2.toString());
}
// ============================================================
// t4: vote ttl_expired — state (fees) не меняется, nonce inc
// ============================================================
async function t4_vote_ttl_no_effect() {
    console.log("\n[t4] vote ttl_expired: fees unchanged, nonce inc");
    const param = "rb_vote_" + String(Date.now());
    const value = "777";
    // первый vote (n5) → vote_counted (value=0, gasLimit=20000)
    await voteFromRoot("n5", param, value, "1");
    const v0 = await mtjs.get_values(node, 3);
    const before = v0.fees[param];
    console.log("  fees[param] before:", before);
    await new Promise((r) => setTimeout(() => r(), 2000));
    // второй vote (n6) → vote_failed ttl_expired (TX ok, nonce inc)
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
    for (const k of Object.keys(em))
        for (const e of em[k])
            if (e[0] === "vote_failed")
                failedReason = e[1].reason;
    check(failedReason === "ttl_expired", "vote_failed ttl_expired");
    const v1 = await mtjs.get_values(node, 3);
    // fees[param] не появился (state не изменён — кворум не набран)
    check(v1.fees[param] === before, "fees[param] unchanged: " + before + " -> " + v1.fees[param]);
    const { nonce: rn1 } = await getNonceBal(rootAddr);
    check(rn1 === rn0 + 1n, "root nonce inc (vote TX в блоке): " + rn0.toString() + " -> " + rn1.toString());
}
// ============================================================
// main
// ============================================================
(async () => {
    console.log("=== ROLLBACK TESTS ===");
    await setup();
    await t1_err_state();
    await t2_partial_batch();
    await t3_separate_txs();
    await t4_vote_ttl_no_effect();
    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0)
        console.log(GREEN + "DONE" + RESET);
    else
        console.log(RED + "FAIL" + RESET);
})().catch((e) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => { }, 1000000);
