#!/usr/local/bin/mtjs
import * as std from "std";
import { node, sk, pk, check, errStr, bootstrap, getNonce, submitAndWait } from "./chk-lib";
// фильтр: ONLY=2 → только [2]; ONLY не задан → все
const only = std.getenv("ONLY");
function want(n) {
    return !only || only === String(n);
}
// ---- сценарии ----
async function t1_nonce() {
    console.log("\n[1] get_user_nonce");
    try {
        const ui = await mtjs.get_user_nonce(node, mtjs.addr_from_pk(pk), 3);
        console.log("  nonce:", ui.nonce, "balance:", ui.balance);
        check(ui.nonce !== undefined, "nonce defined");
    }
    catch (e) {
        check(false, "get_user_nonce: " + errStr(e));
    }
}
async function t2_happy() {
    console.log("\n[2] happy: mint + tx_wait_block");
    try {
        const nonce = await getNonce();
        const { rsp, block } = await submitAndWait(sk, nonce, [{ contract: "root", method: "mint", params: { amount: "1" } }]);
        console.log("  tx_hash:", rsp.tx_hash);
        console.log("  block_hash:", block.block_hash);
        console.log("  finalized:", block.finalized);
        console.log("  status:", block.status(rsp.tx_hash));
        console.log("  emits:", JSON.stringify(block.emits(rsp.tx_hash)));
        check(block.finalized === true, "finalized === true");
        check(block.block_hash.length > 0, "block_hash non-empty");
        check(block.tx_hashes.indexOf(rsp.tx_hash) >= 0, "tx_hashes includes tx");
        check(block.status(rsp.tx_hash) === "ok", "status === ok");
        const em = block.emits(rsp.tx_hash);
        check(!!em && em.length > 0 && em[0][0] === "result", "emits[0][0] === 'result'");
        check(!!em && em[0][1].success === true, "emits[0][1].success === true");
        check(block.errstr(rsp.tx_hash) === null, "errstr === null");
    }
    catch (e) {
        check(false, "happy: " + errStr(e));
    }
}
async function t3_failed() {
    console.log("\n[3] failed TX: bad nonce");
    try {
        const nonce = await getNonce();
        const bad = (nonce + 100000n).toString();
        const req = { tx: [{ contract: "root", method: "mint", params: { amount: "1" } }], nonce: bad, value: 1000000, gasLimit: 100000, gasPrice: 10 };
        const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, bad, () => { });
        console.log("  submit:", rsp.err, rsp.err_str);
        if (rsp.err) {
            console.log("  rejected before block, skip");
            return;
        }
        const block = await mtjs.tx_wait_block(node, rsp.tx_hash, 10);
        console.log("  status:", block.status(rsp.tx_hash));
        console.log("  errstr:", block.errstr(rsp.tx_hash));
        console.log("  emits:", JSON.stringify(block.emits(rsp.tx_hash)));
        check(block.status(rsp.tx_hash) === "err", "status === err");
        check(typeof block.errstr(rsp.tx_hash) === "string", "errstr is string");
    }
    catch (e) {
        check(false, "failed: " + errStr(e));
    }
}
async function t4_timeout() {
    console.log("\n[4] tx_wait_block timeout");
    try {
        const fake = "00".repeat(32);
        await mtjs.tx_wait_block(node, fake, 1);
        check(false, "should have timed out");
    }
    catch (e) {
        console.log("  rejected:", errStr(e));
        check(e && e.errcode === 1, "errcode === 1");
    }
}
async function t5_get_block() {
    console.log("\n[5] get_block (next)");
    try {
        const t0 = Date.now();
        const b = await mtjs.get_block(node, 5);
        const dt = Date.now() - t0;
        console.log("  block_hash:", b.block_hash, "tx_hashes:", b.tx_hashes.length, "in", dt, "ms");
        check(b.block_hash.length > 0, "block_hash non-empty");
        check(dt < 5500, "resolved < 5.5s");
    }
    catch (e) {
        check(false, "get_block: " + errStr(e));
    }
}
async function t6_subscribe() {
    console.log("\n[6] tx_subscribe callback");
    let got = 0;
    mtjs.tx_subscribe(node, (b) => {
        got++;
        if (got === 1) {
            console.log("  cb#1 block_hash:", b.block_hash);
            console.log("  cb#1 tx_hashes:", JSON.stringify(b.tx_hashes));
            check(typeof b.block_hash === "string" && b.block_hash.length > 0, "cb block.block_hash non-empty");
            check(Array.isArray(b.tx_hashes), "cb block.tx_hashes is array");
        }
    });
    const nonce = await getNonce();
    try {
        const req = { tx: [{ contract: "root", method: "mint", params: { amount: "1" } }], nonce: nonce.toString(), value: 1000000, gasLimit: 100000, gasPrice: 10 };
        await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, nonce.toString(), () => { });
        const t0 = Date.now();
        while (got === 0 && Date.now() - t0 < 10000) {
            await new Promise((r) => setTimeout(() => r(), 50));
        }
        check(got > 0, "callback fired");
    }
    catch (e) {
        check(false, "subscribe: " + errStr(e));
    }
}
async function t7_vote_current() {
    console.log("\n[7] vote (без TTL, текущий)");
    try {
        const nonce = await getNonce();
        const param = "vote_test_" + String(Date.now());
        const value = "42";
        console.log("  param:", param);
        const { rsp: r1, block: b1 } = await submitAndWait(sk, nonce, [{ contract: "root", method: "vote", params: { node: "n5", param, value } }]);
        console.log("  [n5] status:", b1.status(r1.tx_hash));
        console.log("  [n5] emits:", JSON.stringify(b1.emits(r1.tx_hash)));
        check(b1.status(r1.tx_hash) === "ok", "n5 vote ok");
        const n2 = await getNonce();
        const { rsp: r2, block: b2 } = await submitAndWait(sk, n2, [
            { contract: "root", method: "vote", params: { node: "n6", param, value } },
            { contract: "root", method: "vote", params: { node: "n7", param, value } },
            { contract: "root", method: "vote", params: { node: "n8", param, value } },
            { contract: "root", method: "vote", params: { node: "n9", param, value } },
        ]);
        const ch = b2.children(r2.tx_hash);
        let applied = false;
        if (ch)
            for (const k of Object.keys(ch)) {
                const emits = ch[k].emits;
                console.log("  child[" + k + "]:", JSON.stringify(emits));
                for (const em of emits)
                    if (em[0] === "vote_applied")
                        applied = true;
            }
        check(applied, "vote_applied present (quorum reached)");
    }
    catch (e) {
        check(false, "vote: " + errStr(e));
    }
}
async function t8_vote_owner() {
    console.log("\n[8] vote от не-владельца (u1)");
    const u1sk = std.getenv("k_u1_ed_sk");
    const u1pk = std.getenv("k_u1_ed_pk");
    if (!u1sk || !u1pk) {
        console.log("  no u1 keys, skip");
        return;
    }
    try {
        const addr = mtjs.addr_from_pk(u1pk);
        const ui = await mtjs.get_user_nonce(node, addr, 3);
        const nonce = BigInt(ui.nonce);
        const param = "vote_owner_" + String(Date.now());
        const req = { tx: [{ contract: "root", method: "vote", params: { node: "n5", param, value: "1" } }], nonce: nonce.toString(), value: 1000000, gasLimit: 100000, gasPrice: 10 };
        const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), u1sk, nonce.toString(), () => { });
        console.log("  submit:", rsp.err, rsp.err_str);
        if (rsp.err)
            return;
        const b = await mtjs.tx_wait_block(node, rsp.tx_hash, 10);
        console.log("  status:", b.status(rsp.tx_hash));
        console.log("  errstr:", b.errstr(rsp.tx_hash));
        check(b.status(rsp.tx_hash) === "err", "status === err");
        const es = b.errstr(rsp.tx_hash);
        check(typeof es === "string" && es.indexOf("node owner") >= 0, "errstr mentions 'node owner'");
    }
    catch (e) {
        check(false, "vote-owner: " + errStr(e));
    }
}
// ---- main ----
async function main() {
    if (!std.getenv("SKIP_BOOTSTRAP"))
        bootstrap();
    if (!sk || !pk)
        throw new Error("no root sk/pk");
    console.log("addr:", mtjs.addr_from_pk(pk));
    if (want(1))
        await t1_nonce();
    if (want(2))
        await t2_happy();
    if (want(3))
        await t3_failed();
    if (want(4))
        await t4_timeout();
    if (want(5))
        await t5_get_block();
    if (want(6))
        await t6_subscribe();
    if (want(7))
        await t7_vote_current();
    if (want(8))
        await t8_vote_owner();
    // summary доступен только при run-all (pass/fail живут в lib)
    const { pass, fail } = await import("./chk-lib");
    console.log(`\n=== RESULT: ${pass} passed, ${fail} failed ===`);
}
main()
    .then(() => console.log("ALL DONE"))
    .catch((e) => console.log("MAIN ERR:", errStr(e)));
setInterval(() => { }, 1000000);
