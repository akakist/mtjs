#!/usr/local/bin/mtjs
import * as std from "std";

const node = "127.0.0.1:2301";
const sk = std.getenv("u_root_ed_sk") || std.getenv("k_root_ed_sk");
const pk = std.getenv("k_root_ed_pk");

const GREEN = "\x1b[32m"; const RED = "\x1b[31m"; const RESET = "\x1b[0m";
let pass = 0; let fail = 0;
function check(cond: boolean, msg: string) {
    if (cond) { pass++; console.log("  " + GREEN + "✅" + RESET + " " + msg); }
    else      { fail++; console.log("  " + RED   + "❌" + RESET + " " + msg); }
}
function errStr(e: any): string {
    if (e && typeof e === "object" && ("errcode" in e || "errstr" in e)) return "errcode=" + e.errcode + " errstr=" + e.errstr;
    return String(e);
}

// ============================================================
// pending tx → block (tx_subscribe до tx_submit)
// ============================================================
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

async function getNonce(): Promise<bigint> {
    const ui = await mtjs.get_user_nonce(node, mtjs.addr_from_pk(pk!), 3);
    return BigInt(ui.nonce);
}

// ждать, пока нода применит блок → nonce > lastNonce
async function waitNonceGt(lastNonce: bigint, timeoutSec = 10): Promise<bigint> {
    const t0 = Date.now();
    while (Date.now() - t0 < timeoutSec * 1000) {
        const n = await getNonce();
        if (n > lastNonce) return n;
        await new Promise<void>((r) => setTimeout(() => r(), 200));
    }
    throw new Error("waitNonceGt timeout: lastNonce=" + lastNonce.toString());
}

// ok TX: submit → wait block → waitNonceGt (nonce+1)
async function submitAndWait(tx: any[], timeoutSec = 10) {
    const nonce = await getNonce();
    const req = { tx, nonce: nonce.toString(), value: 1000000, gasLimit: 100000, gasPrice: 10 };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk!, nonce.toString(), () => {});
    if (rsp.err) throw new Error("submit: " + rsp.err_str);
    const b = await waitTx(rsp.tx_hash, timeoutSec);
    await waitNonceGt(nonce, timeoutSec);
    return { rsp, block: b };
}

// err TX: submit → wait block (nonce не inc, waitNonceGt не нужен)
async function submitAndWaitErr(tx: any[], timeoutSec = 10) {
    const nonce = await getNonce();
    const req = { tx, nonce: nonce.toString(), value: 1000000, gasLimit: 100000, gasPrice: 10 };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk!, nonce.toString(), () => {});
    if (rsp.err) throw new Error("submit: " + rsp.err_str);
    const b = await waitTx(rsp.tx_hash, timeoutSec);
    return { rsp, block: b };
}

function childEmits(b: Block, txHash: string): { [seq: string]: any[] } {
    const ch = b.children(txHash);
    const out: { [seq: string]: any[] } = {};
    if (ch) for (const k of Object.keys(ch)) out[k] = (ch as any)[k].emits;
    return out;
}
function hasEmit(em: { [seq: string]: any[] }, name: string): any | null {
    for (const k of Object.keys(em)) for (const e of em[k]) if (e[0] === name) return e[1];
    return null;
}

// ============================================================
// vote по одной TX (n0..n9), пока не наберётся кворум (vote_applied)
// ============================================================
async function voteUntilApplied(param: string, value: string, ttl: string) {
    const nodes = ["n0","n1","n2","n3","n4","n5","n6","n7","n8","n9"];
    for (const vnode of nodes) {
        const { rsp, block } = await submitAndWait([
            { contract: "root", method: "vote", params: { node: vnode, param, value, ttl } },
        ]);
        const em = childEmits(block, rsp.tx_hash);
        const st = block.status(rsp.tx_hash);
        if (st !== "ok") return { ok: false, err: "status=" + st + " errstr=" + block.errstr(rsp.tx_hash), node: vnode, em };
        const applied = hasEmit(em, "vote_applied");
        const counted = hasEmit(em, "vote_counted");
        const failed  = hasEmit(em, "vote_failed");
        if (applied) return { ok: true, applied, node: vnode, em };
        if (counted) { console.log("  [" + vnode + "] vote_counted → continue"); continue; }
        if (failed) {
            if (failed.reason === "ttl_expired") {
                console.log("  [" + vnode + "] vote_failed (ttl_expired) → erase, continue");
                continue;
            }
            return { ok: false, err: "vote_failed: " + JSON.stringify(failed), node: vnode, em };
        }
        return { ok: false, err: "no vote_counted/applied/failed", node: vnode, em };
    }
    return { ok: false, err: "no vote_applied after all nodes" };
}

// ============================================================
// test 1: vote_applied (quorum)
// ============================================================
async function t1_vote_applied() {
    console.log("\n[t1] vote_applied (quorum, sequential)");
    const param = "vote_test_" + String(Date.now());
    const value = "42";
    const r = await voteUntilApplied(param, value, "60");
    check(r.ok, "vote_applied reached (quorum)");
    if (!r.ok) { console.log("  err: " + r.err); return; }
    check(r.applied.param === param && r.applied.value === value, "vote_applied param/value");
}

// ============================================================
// test 2: TTL expire → vote_failed
// ============================================================
async function t2_vote_expire() {
    console.log("\n[t2] vote_failed (ttl_expired)");
    const param = "vote_exp_" + String(Date.now());
    const value = "99";

    // первый vote (n5, ttl=1) → vote_counted
    const { rsp: r1, block: b1 } = await submitAndWait([
        { contract: "root", method: "vote", params: { node: "n5", param, value, ttl: "1" } },
    ]);
    check(b1.status(r1.tx_hash) === "ok", "first status ok");
    check(hasEmit(childEmits(b1, r1.tx_hash), "vote_counted") !== null, "vote_counted present");

    // vote n6 (промежуток: elapsed n5→n6 < 1 → vote_counted; если уже vote_failed — ok, пропускаем)
    const { rsp: r6, block: b6 } = await submitAndWait([
        { contract: "root", method: "vote", params: { node: "n6", param, value, ttl: "1" } },
    ]);
    check(b6.status(r6.tx_hash) === "ok", "n6 vote ok");
    if (hasEmit(childEmits(b6, r6.tx_hash), "vote_failed")) {
        console.log("  n6 уже vote_failed (elapsed > ttl), ok");
        return;
    }

    await new Promise<void>((r) => setTimeout(() => r(), 2000));

    // второй vote (n6, ttl=1) → vote_failed
    const { rsp: r2, block: b2 } = await submitAndWait([
        { contract: "root", method: "vote", params: { node: "n6", param, value, ttl: "1" } },
    ]);
    check(b2.status(r2.tx_hash) === "ok", "second status ok (tx success, vote failed)");
    const failed = hasEmit(childEmits(b2, r2.tx_hash), "vote_failed");
    check(failed !== null, "vote_failed present");
    if (failed) {
        check(failed.reason === "ttl_expired", "reason ttl_expired");
        check(Number(failed.elapsed) > Number(failed.ttl), "elapsed > ttl");
    }
}

// ============================================================
// test 3: ttl обязателен на первом vote (err TX — nonce не inc)
// ============================================================
async function t3_ttl_required() {
    console.log("\n[t3] ttl required on first vote");
    const param = "vote_nottl_" + String(Date.now());
    const { rsp, block } = await submitAndWaitErr([
        { contract: "root", method: "vote", params: { node: "n5", param, value: "7" } }, // без ttl
    ]);
    check(block.status(rsp.tx_hash) === "err", "status err (ttl missing)");
    const es = block.errstr(rsp.tx_hash);
    check(typeof es === "string" && es.indexOf("ttl") >= 0, "errstr mentions ttl");
    if (!(typeof es === "string" && es.indexOf("ttl") >= 0)) {
        console.log("  emits:", JSON.stringify(block.emits(rsp.tx_hash)));
    }
}

// ============================================================
// test 4: не-владелец ноды → err (налить u1, value=0 — vote не тратит value)
// ============================================================
async function t4_vote_not_owner() {
    console.log("\n[t4] vote from non-owner");
    const u1sk = std.getenv("k_u1_ed_sk");
    const u1pk = std.getenv("k_u1_ed_pk");
    if (!u1sk || !u1pk) { console.log("  (skip: no k_u1_* keys)"); return; }
    const u1addr = mtjs.addr_from_pk(u1pk);

    // налить u1 баланс: root mint 1000000 → root transfer u1 1000000
    await submitAndWait([{ contract: "root", method: "mint", params: { amount: "1000000" } }]);
    await submitAndWait([{ contract: "root", method: "transfer", params: { to: u1addr, amount: "1000000" } }]);
    console.log("  u1 balance filled");

    const ui = await mtjs.get_user_nonce(node, u1addr, 3);
    const nonce = BigInt(ui.nonce);
    const param = "vote_owner_" + String(Date.now());
    const tx = [{ contract: "root", method: "vote", params: { node: "n5", param, value: "1", ttl: "60" } }];
    // value: 0 — vote не тратит value; u1 1000000 >= gasLimit*gasPrice=1,000,000 → ok → execute_vote → owner-ошибка
    const req = { tx, nonce: nonce.toString(), value: 0, gasLimit: 100000, gasPrice: 10 };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), u1sk, nonce.toString(), () => {});
    if (rsp.err) { check(false, "submit: " + rsp.err_str); return; }
    const b = await waitTx(rsp.tx_hash, 10);
    check(b.status(rsp.tx_hash) === "err", "status err (not owner)");
    const es = b.errstr(rsp.tx_hash);
    check(typeof es === "string" && es.indexOf("owner") >= 0, "errstr mentions owner");
    if (!(typeof es === "string" && es.indexOf("owner") >= 0)) {
        console.log("  emits:", JSON.stringify(b.emits(rsp.tx_hash)));
    }
}

// ============================================================
// test 5: introduce validator_min_stake (governance) + activated
// ============================================================
async function t5_introduce_param() {
    console.log("\n[t5] introduce validator_min_stake via vote (sequential)");
    const v0 = await mtjs.get_values(node, 3);
    console.log("  before: validator_min_stake =", v0.fees["validator_min_stake"]);

    const param = "validator_min_stake";
    const value = "100";
    const r = await voteUntilApplied(param, value, "60");
    check(r.ok, "vote_applied reached (quorum)");
    if (!r.ok) { console.log("  err: " + r.err); return; }

    const v1 = await mtjs.get_values(node, 3);
    console.log("  after: validator_min_stake =", v1.fees["validator_min_stake"]);
    check(v1.fees["validator_min_stake"] === "100", "validator_min_stake === '100'");
    check(v1.fees["validator_min_stake_activated"] === "1", "validator_min_stake_activated === '1'");
}

// ============================================================
// main
// ============================================================
(async () => {
    console.log("=== VOTE ALL TESTS ===");
    if (!sk || !pk) { console.log(RED + "no sk/pk" + RESET); return; }

    await t1_vote_applied();
    await t2_vote_expire();
    await t3_ttl_required();
    await t4_vote_not_owner();
    await t5_introduce_param();

    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0) console.log(GREEN + "DONE" + RESET);
    else            console.log(RED + "FAIL" + RESET);
})().catch((e: any) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => {}, 1000000);
export {};
