#!/usr/local/bin/mtjs
import * as std from "std";

const node = "127.0.0.1:2301";
const KEYS_PATH = std.getenv("MTJS_KEYS_PATH") || "keys_test.json";

let sk = ""; let pk = "";
let u1sk = ""; let u1pk = "";
async function loadKeys() {
    const raw = await mtjs.fs.readFile(KEYS_PATH);
    const keys = JSON.parse(raw);
    const r = keys.users?.root?.keys?.[0];
    if (!r?.sk || !r?.pk) throw new Error("keys_test.json: no users.root.keys[0].sk/pk");
    sk = r.sk; pk = r.pk;
    const u = keys.users?.u1?.keys?.[0];
    if (u?.sk && u?.pk) { u1sk = u.sk; u1pk = u.pk; }
}

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
async function waitNonceGt(addr: string, lastNonce: bigint, timeoutSec = 10): Promise<bigint> {
    const t0 = Date.now();
    while (Date.now() - t0 < timeoutSec * 1000) {
        const n = await getNonce(addr);
        if (n > lastNonce) return n;
        await new Promise<void>((r) => setTimeout(() => r(), 200));
    }
    throw new Error("waitNonceGt timeout: lastNonce=" + lastNonce.toString());
}

const rootAddr = mtjs.addr_from_name("root");
const u1Addr   = mtjs.addr_from_name("u1");

async function submitAndWait(skVal: string, addr: string, tx: any[], value = 0, gasLimit = 100000, retries = 5) {
    for (let attempt = 0; attempt < retries; attempt++) {
        const nonce = await getNonce(addr);
        const req = { tx, nonce: nonce.toString(), value, gasLimit, priority: 0, sender: addr, key_id: 0 };
        const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), skVal, nonce.toString(), () => {});
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

async function submitAndWaitErr(skVal: string, addr: string, tx: any[], value = 0, gasLimit = 100000, retries = 5) {
    for (let attempt = 0; attempt < retries; attempt++) {
        const nonce = await getNonce(addr);
        const req = { tx, nonce: nonce.toString(), value, gasLimit, priority: 0, sender: addr, key_id: 0 };
        const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), skVal, nonce.toString(), () => {});
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

async function voteUntilApplied(param: string, value: string, ttl: string) {
    const nodes = ["n0","n1","n2","n3","n4","n5","n6","n7","n8","n9"];
    for (const vnode of nodes) {
        const { rsp, block } = await submitAndWait(sk, rootAddr, [
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

async function t1_vote_applied() {
    console.log("\n[t1] vote_applied (quorum, sequential)");
    const param = "vote_test_" + String(Date.now());
    const value = "42";
    const r = await voteUntilApplied(param, value, "60");
    check(r.ok, "vote_applied reached (quorum)");
    if (!r.ok) { console.log("  err: " + r.err); return; }
    check(r.applied.param === param && r.applied.value === value, "vote_applied param/value");
}

async function t2_vote_expire() {
    console.log("\n[t2] vote_failed (ttl_expired)");
    const param = "vote_exp_" + String(Date.now());
    const value = "99";
    const { rsp: r1, block: b1 } = await submitAndWait(sk, rootAddr, [
        { contract: "root", method: "vote", params: { node: "n5", param, value, ttl: "1" } },
    ]);
    check(b1.status(r1.tx_hash) === "ok", "first status ok");
    check(hasEmit(childEmits(b1, r1.tx_hash), "vote_counted") !== null, "vote_counted present");
    const { rsp: r6, block: b6 } = await submitAndWait(sk, rootAddr, [
        { contract: "root", method: "vote", params: { node: "n6", param, value, ttl: "1" } },
    ]);
    check(b6.status(r6.tx_hash) === "ok", "n6 vote ok");
    if (hasEmit(childEmits(b6, r6.tx_hash), "vote_failed")) {
        console.log("  n6 уже vote_failed (elapsed > ttl), ok");
        return;
    }
    await new Promise<void>((r) => setTimeout(() => r(), 2000));
    const { rsp: r2, block: b2 } = await submitAndWait(sk, rootAddr, [
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

async function t3_ttl_required() {
    console.log("\n[t3] ttl required on first vote");
    const param = "vote_nottl_" + String(Date.now());
    const { rsp, block } = await submitAndWaitErr(sk, rootAddr, [
        { contract: "root", method: "vote", params: { node: "n5", param, value: "7" } },
    ]);
    check(block.status(rsp.tx_hash) === "err", "status err (ttl missing)");
    const es = block.errstr(rsp.tx_hash);
    check(typeof es === "string" && es.indexOf("ttl") >= 0, "errstr mentions ttl");
    if (!(typeof es === "string" && es.indexOf("ttl") >= 0)) {
        console.log("  emits:", JSON.stringify(block.emits(rsp.tx_hash)));
    }
}

async function t4_vote_not_owner() {
    console.log("\n[t4] vote from non-owner");
    if (!u1sk || !u1pk) { console.log("  (skip: no u1 keys)"); return; }

    await submitAndWait(sk, rootAddr, [{ contract: "root", method: "mint", params: { amount: "1000000" } }]);
    await submitAndWait(sk, rootAddr, [{ contract: "root", method: "transfer", params: { to: u1Addr, amount: "1000000" } }]);
    console.log("  u1 balance filled");

    const param = "vote_owner_" + String(Date.now());
    const { rsp, block } = await submitAndWaitErr(u1sk, u1Addr, [
        { contract: "root", method: "vote", params: { node: "n5", param, value: "1", ttl: "60" } },
    ], 0, 100000);
    check(block.status(rsp.tx_hash) === "err", "status err (not owner)");
    const es = block.errstr(rsp.tx_hash);
    check(typeof es === "string" && es.indexOf("owner") >= 0, "errstr mentions owner");
    if (!(typeof es === "string" && es.indexOf("owner") >= 0)) {
        console.log("  emits:", JSON.stringify(block.emits(rsp.tx_hash)));
    }
}

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

(async () => {
    await loadKeys();
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
    std.exit(fail);
})().catch((e: any) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => {}, 1000000);
export {};