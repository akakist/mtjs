#!/usr/local/bin/mtjs
import * as std from "std";
const node = "127.0.0.1:2301";
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
const rootAddr = mtjs.addr_from_pk(rootPk);
async function getState() {
    return await mtjs.get_state(node, 3);
}
async function main() {
    console.log("=== STATE TEST (node " + node + ") ===");
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
    const s0 = await getState();
    console.log("  state0:", JSON.stringify(s0));
    check(s0.found === true, "state found");
    check(s0.chain_id === 1, "chain_id == 1");
    check(s0.genesis_time > 0, "genesis_time > 0");
    check(s0.node_count >= 10, "node_count >= 10 (init_root)");
    check(typeof s0.total_supply === "string", "total_supply string");
    const supply0 = BigInt(s0.total_supply);
    const tx0 = s0.tx_count;
    const bh0 = s0.block_height;
    // t1: mint 1000 → total_supply +1000
    {
        console.log("\n[t1] mint 1000 → total_supply +1000");
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "root", method: "mint", params: { amount: "1000" } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "mint 1000 ok (errstr=" + (block.errstr(rsp.tx_hash) || "") + ")");
        const s1 = await getState();
        const supply1 = BigInt(s1.total_supply);
        check(supply1 === supply0 + 1000n, "total_supply +1000: " + supply0 + " -> " + supply1);
        check(s1.tx_count > tx0, "tx_count grew: " + tx0 + " -> " + s1.tx_count);
        check(s1.block_height > bh0, "block_height grew: " + bh0 + " -> " + s1.block_height);
    }
    // t2: transfer (ok) → tx_count +, block_height +
    {
        console.log("\n[t2] transfer (ok) → tx_count/block_height");
        const s1 = await getState();
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "root", method: "transfer", params: { to: rootAddr, amount: "1" } },
        ], 100000, 20000); // value >= amount
        const s2 = await getState();
        // transfer to self — err ("cannot transfer to self") — TBD; берём ok или err — tx_count++
        check(s2.tx_count > s1.tx_count, "tx_count grew: " + s1.tx_count + " -> " + s2.tx_count);
    }
    // t3: contract_deploy → contract_count++
    {
        console.log("\n[t3] contract_deploy → contract_count++");
        const s1 = await getState();
        const src = "registerMethod('noop', function(p, m) {});";
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "root", method: "contract_deploy", params: { name: "state_test_" + Date.now(), src } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "deploy ok (errstr=" + (block.errstr(rsp.tx_hash) || "") + ")");
        const s2 = await getState();
        check(s2.contract_count > s1.contract_count, "contract_count grew: " + s1.contract_count + " -> " + s2.contract_count);
    }
    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0)
        console.log(GREEN + "DONE" + RESET);
    else
        console.log(RED + "FAIL" + RESET);
}
main().catch((e) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => { }, 1000000);
