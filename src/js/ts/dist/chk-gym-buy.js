#!/usr/local/bin/mtjs
import * as std from "std";
// chk-gym-buy.ts — gym buyVisits за нативный токен (value):
//   buyVisits(visits) → consumeValue(visits * PRICE_PER_VISIT), visits += n, emit PassIssued {paid}
//   err (consumeValue insufficient value) → state откатан (visits), value возврат (rollback), газ списан.
// node :2301 (n1). root — admin + юзер (hex).
const node = "127.0.0.1:2301";
const GYM_SRC_PATH = "/home/bsr/mtjs/src/js/tsi/dist/contracts/gym.js";
const GAS_LIMIT = 100000;
const PRICE_PER_VISIT = 10;
const rootSk = std.getenv("u_root_ed_sk") || std.getenv("k_root_ed_sk");
const rootPk = std.getenv("k_root_ed_pk");
if (!rootSk || !rootPk)
    throw new Error("no root sk/pk");
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
async function getNonce(addr) {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    return BigInt(ui.nonce);
}
async function getBalance(addr) {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    return BigInt(ui.balance);
}
async function getNonceBal(addr) {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    return { nonce: BigInt(ui.nonce), bal: BigInt(ui.balance) };
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
async function submitAndWait(sk, addr, tx, value = 0, gasLimit = GAS_LIMIT) {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    const req = { tx, nonce: ui.nonce, value, gasLimit, gasPrice: 10 };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, ui.nonce, () => { });
    if (rsp.err)
        throw new Error("submit: " + rsp.err_str);
    const b = await waitTx(rsp.tx_hash, 10);
    await waitNonceGt(addr, BigInt(ui.nonce), 10);
    return { rsp, block: b };
}
async function submitAndWaitErr(sk, addr, tx, value = 0, gasLimit = GAS_LIMIT) {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    const req = { tx, nonce: ui.nonce, value, gasLimit, gasPrice: 10 };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, ui.nonce, () => { });
    if (rsp.err)
        throw new Error("submit: " + rsp.err_str);
    const b = await waitTx(rsp.tx_hash, 10);
    await waitNonceGt(addr, BigInt(ui.nonce), 10);
    return { rsp, block: b };
}
const rootAddr = mtjs.addr_from_pk(rootPk);
async function ensureRootFunded(minBalance) {
    const bal = await getBalance(rootAddr);
    console.log("  root balance:", bal.toString(), "min:", minBalance.toString());
    if (bal >= minBalance) {
        check(true, "root funded >= " + minBalance.toString());
        return;
    }
    if (bal < 200000n) {
        check(false, "root balance too low for mint — clean db");
        return;
    }
    const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
        { contract: "root", method: "mint", params: { amount: "100000000" } },
    ], 0, 20000);
    check(block.status(rsp.tx_hash) === "ok", "mint root 100M ok");
}
async function ensureGym(src) {
    const c = await mtjs.get_contract(node, "gym", 3);
    if (c.found) {
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "root", method: "contract_update", params: { name: "gym", src } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "contract_update gym ok");
    }
    else {
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "root", method: "contract_deploy", params: { name: "gym", src } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "contract_deploy gym ok");
    }
}
async function ensureInit() {
    const r = await mtjs.get_contract_data(node, "gym", "admin", 3);
    if (r.found) {
        check(true, "gym already init");
        return;
    }
    const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
        { contract: "gym", method: "init", params: { admin: rootAddr } },
    ]);
    check(block.status(rsp.tx_hash) === "ok", "gym.init ok");
}
async function getVisits() {
    const r = await mtjs.get_contract_data(node, "gym", "abonement" + rootAddr, 3);
    if (!r.found)
        return -1;
    return Number(JSON.parse(r.value).visits);
}
function childEmits(block, txHash, seqId) {
    const ch = block.children(txHash);
    if (!ch)
        return [];
    const node = ch[String(seqId)];
    if (!node || !node.emits)
        return [];
    const out = [];
    for (const e of node.emits) {
        let val = e[1];
        if (typeof val === "string") {
            try {
                val = JSON.parse(val);
            }
            catch (_) { }
        }
        out.push({ name: e[0], value: val });
    }
    return out;
}
function findEmit(list, name) {
    for (const e of list)
        if (e.name === name)
            return e.value;
    return null;
}
async function main() {
    console.log("=== GYM BUY (node " + node + ") ===");
    await ensureRootFunded(50000000n);
    let src = "";
    try {
        src = await mtjs.fs.readFile(GYM_SRC_PATH);
    }
    catch (e) {
        check(false, "read gym src: " + errStr(e));
        return;
    }
    check(src.length > 0, "gym src read (" + src.length + " bytes)");
    await ensureGym(src);
    await ensureInit();
    // 1) buyVisits(5) value=50 → visits += 5, emit PassIssued {paid:50}
    {
        console.log("\n[1] buyVisits(5) value=50 → visits+=5, emit PassIssued");
        const visits0 = await getVisits();
        const { nonce: n0, bal: b0 } = await getNonceBal(rootAddr);
        const total = 5 * PRICE_PER_VISIT;
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "gym", method: "buyVisits", params: { visits: 5 } },
        ], total);
        check(block.status(rsp.tx_hash) === "ok", "buyVisits ok (errstr=" + block.errstr(rsp.tx_hash) + ")");
        const visits1 = await getVisits();
        const v0 = visits0 < 0 ? 0 : visits0;
        check(visits1 === v0 + 5, "visits +5: " + v0 + " -> " + visits1);
        const em = childEmits(block, rsp.tx_hash, 0);
        console.log("  emits[0]:", JSON.stringify(em));
        const pi = findEmit(em, "PassIssued");
        check(pi !== null, "PassIssued present");
        if (pi) {
            check(String(pi.user) === rootAddr, "PassIssued.user == root");
            check(Number(pi.paid) === total, "PassIssued.paid == " + total + " (got " + pi.paid + ")");
        }
        const { nonce: n1, bal: b1 } = await getNonceBal(rootAddr);
        check(n1 === n0 + 1n, "nonce +1");
        check(b1 <= b0 - BigInt(total), "balance <= b0 - " + total + " (value + gas): " + b0.toString() + " -> " + b1.toString());
    }
    // 2) err: buyVisits(5) value=0 → consumeValue insufficient value → err, visits не изменился, газ списан
    {
        console.log("\n[2] buyVisits(5) value=0 → consumeValue insufficient value (err)");
        const visits0 = await getVisits();
        const { nonce: n0, bal: b0 } = await getNonceBal(rootAddr);
        const { rsp, block } = await submitAndWaitErr(rootSk, rootAddr, [
            { contract: "gym", method: "buyVisits", params: { visits: 5 } },
        ], 0);
        check(block.status(rsp.tx_hash) === "err", "status err");
        const es = block.errstr(rsp.tx_hash) || "";
        console.log("  errstr:", es);
        check(es.indexOf("insufficient value") >= 0, "errstr mentions insufficient value");
        const visits1 = await getVisits();
        check(visits1 === visits0, "visits unchanged: " + visits0 + " -> " + visits1);
        const { nonce: n1, bal: b1 } = await getNonceBal(rootAddr);
        check(n1 === n0 + 1n, "nonce +1 (err TX)");
        check(b1 <= b0, "balance not increased (gas charged): " + b0.toString() + " -> " + b1.toString());
    }
    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0)
        console.log(GREEN + "DONE" + RESET);
    else
        console.log(RED + "FAIL" + RESET);
}
main().catch((e) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => { }, 1000000);
