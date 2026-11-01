#!/usr/local/bin/mtjs
import * as std from "std";
// chk-gym-emits.ts — эмиты gym на живой ноде:
//   issuePass → PassIssued
//   createEntry → EntryCreated
//   markUsed (used<M) → EntryUsed
//   markUsed (used==M) → EntryClosed (pass splice)
//   cancelEntry → EntryCancelled
//   disableEntry → EntryDisabled
// эмиты в block.children(tx_hash)[seqId].emits (не root-level).
// node :2301 (n1). root — admin/sender (hex).
const node = "127.0.0.1:2301";
const GYM_SRC_PATH = "/home/bsr/mtjs/src/js/tsi/dist/contracts/gym.js";
const GAS_LIMIT = 100000;
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
    const req = { tx, nonce: ui.nonce, value, gasLimit, priority: 0 };
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
async function getAbonement() {
    const r = await mtjs.get_contract_data(node, "gym", "abonement" + rootAddr, 3);
    if (!r.found)
        return -1;
    return Number(JSON.parse(r.value).visits);
}
async function getPasses() {
    const r = await mtjs.get_contract_data(node, "gym", "entry" + rootAddr, 3);
    if (!r.found)
        return [];
    return JSON.parse(r.value).passes || [];
}
// ищем emits команды index в block.children(tx_hash)[seqId].emits → [{name, value}]
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
    console.log("=== GYM EMITS (node " + node + ") ===");
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
    // visits >= 10
    {
        const v = await getAbonement();
        if (v < 10) {
            const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
                { contract: "gym", method: "issuePass", params: { visits: 10 - Math.max(v, 0) } },
            ]);
            check(block.status(rsp.tx_hash) === "ok", "issuePass top-up ok");
        }
        else
            check(true, "abonement.visits >= 10 (skip issuePass)");
    }
    // 1) issuePass → PassIssued (в block.children, seqId=0)
    {
        console.log("\n[1] issuePass → PassIssued");
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "gym", method: "issuePass", params: { visits: 3 } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "issuePass ok");
        const em = childEmits(block, rsp.tx_hash, 0);
        console.log("  emits[0]:", JSON.stringify(em));
        const pi = findEmit(em, "PassIssued");
        check(pi !== null, "PassIssued present in children[0].emits");
        if (pi) {
            check(String(pi.user) === rootAddr, "PassIssued.user == root");
            check(Number(pi.visits) === 3, "PassIssued.visits == 3 (got " + pi.visits + ")");
        }
        // root-level emits — только result
        const rootEm = block.emits(rsp.tx_hash) || [];
        check(rootEm.length >= 1, "root-level emits (result) present");
    }
    // 2) createEntry → EntryCreated
    {
        console.log("\n[2] createEntry → EntryCreated");
        // если есть активный — сначала cancel/disable старых (или создаём новый — политика)
        const passes = await getPasses();
        const now = Math.floor(Date.now() / 1000);
        let active = null;
        for (const pp of passes)
            if (!pp.disabled && Number(pp.used) < Number(pp.M) && Number(pp.expires_at) > now) {
                active = pp;
                break;
            }
        if (active) {
            // disable активный, чтобы createEntry прошёл
            await submitAndWait(rootSk, rootAddr, [
                { contract: "gym", method: "disableEntry", params: { user: rootAddr, entry_id: Number(active.id) } },
            ]);
        }
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "gym", method: "createEntry", params: { M: 2, ttl: 300 } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "createEntry ok");
        const em = childEmits(block, rsp.tx_hash, 0);
        console.log("  emits[0]:", JSON.stringify(em));
        const ec = findEmit(em, "EntryCreated");
        check(ec !== null, "EntryCreated present");
        if (ec) {
            check(String(ec.user) === rootAddr, "EntryCreated.user == root");
            check(Number(ec.M) === 2, "EntryCreated.M == 2 (got " + ec.M + ")");
        }
    }
    // 3) markUsed (used<M) → EntryUsed
    let activeId = -1;
    {
        console.log("\n[3] markUsed (count=1, used<M) → EntryUsed");
        const passes = await getPasses();
        const now = Math.floor(Date.now() / 1000);
        for (const pp of passes)
            if (!pp.disabled && Number(pp.used) < Number(pp.M) && Number(pp.expires_at) > now) {
                activeId = Number(pp.id);
                break;
            }
        check(activeId >= 0, "active entry found (id=" + activeId + ")");
        if (activeId >= 0) {
            const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
                { contract: "gym", method: "markUsed", params: { user: rootAddr, entry_id: activeId, count: 1 } },
            ]);
            check(block.status(rsp.tx_hash) === "ok", "markUsed ok");
            const em = childEmits(block, rsp.tx_hash, 0);
            console.log("  emits[0]:", JSON.stringify(em));
            const eu = findEmit(em, "EntryUsed");
            check(eu !== null, "EntryUsed present");
            if (eu) {
                check(String(eu.user) === rootAddr, "EntryUsed.user == root");
                check(Number(eu.id) === activeId, "EntryUsed.id == " + activeId);
                check(Number(eu.used) === 1, "EntryUsed.used == 1 (got " + eu.used + ")");
            }
        }
    }
    // 4) markUsed (count=M-used → used==M) → EntryClosed
    if (activeId >= 0) {
        console.log("\n[4] markUsed (count = M-used → used==M) → EntryClosed");
        const passes = await getPasses();
        let pp = null;
        for (const x of passes)
            if (Number(x.id) === activeId) {
                pp = x;
                break;
            }
        check(!!pp, "active entry present (id=" + activeId + ")");
        if (pp) {
            const remain = Number(pp.M) - Number(pp.used);
            const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
                { contract: "gym", method: "markUsed", params: { user: rootAddr, entry_id: activeId, count: remain } },
            ]);
            check(block.status(rsp.tx_hash) === "ok", "markUsed (count=" + remain + ") ok");
            const em = childEmits(block, rsp.tx_hash, 0);
            console.log("  emits[0]:", JSON.stringify(em));
            const eclosed = findEmit(em, "EntryClosed");
            check(eclosed !== null, "EntryClosed present (used==M → splice)");
            if (eclosed)
                check(Number(eclosed.id) === activeId, "EntryClosed.id == " + activeId);
        }
    }
    // 5) cancelEntry → EntryCancelled
    {
        console.log("\n[5] createEntry + cancelEntry → EntryCancelled");
        // создаём новый entry, отменяем
        const { rsp: rspC, block: blockC } = await submitAndWait(rootSk, rootAddr, [
            { contract: "gym", method: "createEntry", params: { M: 1, ttl: 300 } },
        ]);
        check(blockC.status(rspC.tx_hash) === "ok", "createEntry (M=1) ok");
        const emC = childEmits(blockC, rspC.tx_hash, 0);
        const ec = findEmit(emC, "EntryCreated");
        check(ec !== null, "EntryCreated present");
        if (ec) {
            const id = Number(ec.id);
            const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
                { contract: "gym", method: "cancelEntry", params: { user: rootAddr, entry_id: id } },
            ]);
            check(block.status(rsp.tx_hash) === "ok", "cancelEntry ok");
            const em = childEmits(block, rsp.tx_hash, 0);
            console.log("  emits[0]:", JSON.stringify(em));
            const ecancel = findEmit(em, "EntryCancelled");
            check(ecancel !== null, "EntryCancelled present");
            if (ecancel) {
                check(Number(ecancel.id) === id, "EntryCancelled.id == " + id);
                check(Number(ecancel.refund) === 1, "EntryCancelled.refund == 1 (M=1, used=0)");
            }
        }
    }
    // 6) disableEntry → EntryDisabled
    {
        console.log("\n[6] createEntry + disableEntry → EntryDisabled");
        const { rsp: rspC, block: blockC } = await submitAndWait(rootSk, rootAddr, [
            { contract: "gym", method: "createEntry", params: { M: 1, ttl: 300 } },
        ]);
        check(blockC.status(rspC.tx_hash) === "ok", "createEntry (M=1) ok");
        const emC = childEmits(blockC, rspC.tx_hash, 0);
        const ec = findEmit(emC, "EntryCreated");
        check(ec !== null, "EntryCreated present");
        if (ec) {
            const id = Number(ec.id);
            const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
                { contract: "gym", method: "disableEntry", params: { user: rootAddr, entry_id: id } },
            ]);
            check(block.status(rsp.tx_hash) === "ok", "disableEntry ok");
            const em = childEmits(block, rsp.tx_hash, 0);
            console.log("  emits[0]:", JSON.stringify(em));
            const edis = findEmit(em, "EntryDisabled");
            check(edis !== null, "EntryDisabled present");
            if (edis)
                check(Number(edis.id) === id, "EntryDisabled.id == " + id);
        }
    }
    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0)
        console.log(GREEN + "DONE" + RESET);
    else
        console.log(RED + "FAIL" + RESET);
}
main().catch((e) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => { }, 1000000);
