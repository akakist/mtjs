#!/usr/local/bin/mtjs
import * as std from "std";

// chk-gym-2nodes.ts — gym на двух нодах: block_hash same на :2301/:2302 (err TX не разошёлся).
//   t1: ok TX (issuePass) — block_hash same на :2301/:2302
//   t2: err TX (createEntry no visits) — block_hash same на :2301/:2302 (root hash сошёлся)
// node :2301 (n1), :2302 (n2). root — admin/sender (hex).

const node1 = "127.0.0.1:2301";
const node2 = "127.0.0.1:2302";
const GYM_SRC_PATH = "/home/bsr/mtjs/src/js/tsi/dist/contracts/gym.js";
const GAS_LIMIT = 100000;

const rootSk = std.getenv("u_root_ed_sk") || std.getenv("k_root_ed_sk");
const rootPk = std.getenv("k_root_ed_pk");
if (!rootSk || !rootPk) throw new Error("no root sk/pk");

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

// pending tx → block (по :2301)
const pending: { [h: string]: { resolve: (b: Block) => void; reject: (e: any) => void; timer: any } } = {};
mtjs.tx_subscribe(node1, (b: Block) => {
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
    const ui = await mtjs.get_user_nonce(node1, addr, 3);
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
async function submitAndWait(sk: string, addr: string, tx: any[], value = 0, gasLimit = GAS_LIMIT) {
    const ui = await mtjs.get_user_nonce(node1, addr, 3);
    const req = { tx, nonce: ui.nonce, value, gasLimit, priority: 0 };
    const rsp = await mtjs.tx_submit(node1, 1, JSON.stringify(req), sk, ui.nonce, () => {});
    if (rsp.err) throw new Error("submit: " + rsp.err_str);
    const b = await waitTx(rsp.tx_hash, 10);
    await waitNonceGt(addr, BigInt(ui.nonce), 10);
    return { rsp, block: b };
}
async function submitAndWaitErr(sk: string, addr: string, tx: any[], value = 0, gasLimit = GAS_LIMIT) {
    const ui = await mtjs.get_user_nonce(node1, addr, 3);
    const req = { tx, nonce: ui.nonce, value, gasLimit, priority: 0 };
    const rsp = await mtjs.tx_submit(node1, 1, JSON.stringify(req), sk, ui.nonce, () => {});
    if (rsp.err) throw new Error("submit: " + rsp.err_str);
    const b = await waitTx(rsp.tx_hash, 10);
    await waitNonceGt(addr, BigInt(ui.nonce), 10);
    return { rsp, block: b };
}

const rootAddr = mtjs.addr_from_pk(rootPk!);

async function getBalance(addr: string): Promise<bigint> {
    const ui = await mtjs.get_user_nonce(node1, addr, 3);
    return BigInt(ui.balance);
}
async function ensureRootFunded(minBalance: bigint) {
    const bal = await getBalance(rootAddr);
    console.log("  root balance:", bal.toString(), "min:", minBalance.toString());
    if (bal >= minBalance) { check(true, "root funded >= " + minBalance.toString()); return; }
    if (bal < 200000n) {
        check(false, "root balance too low for mint (" + bal.toString() + " < 200000) — clean db");
        return;
    }
    const { rsp, block } = await submitAndWait(rootSk!, rootAddr, [
        { contract: "root", method: "mint", params: { amount: "100000000" } },
    ], 0, 20000);
    check(block.status(rsp.tx_hash) === "ok", "mint root 100M ok");
}
async function ensureGym(src: string) {
    const c = await mtjs.get_contract(node1, "gym", 3);
    if (c.found) {
        const { rsp, block } = await submitAndWait(rootSk!, rootAddr, [
            { contract: "root", method: "contract_update", params: { name: "gym", src } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "contract_update gym ok (errstr=" + block.errstr(rsp.tx_hash) + ")");
    } else {
        const { rsp, block } = await submitAndWait(rootSk!, rootAddr, [
            { contract: "root", method: "contract_deploy", params: { name: "gym", src } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "contract_deploy gym ok (errstr=" + block.errstr(rsp.tx_hash) + ")");
    }
}
async function ensureInit() {
    const r = await mtjs.get_contract_data(node1, "gym", "admin", 3);
    if (r.found) { check(true, "gym already init (admin found)"); return; }
    const { rsp, block } = await submitAndWait(rootSk!, rootAddr, [
        { contract: "gym", method: "init", params: { admin: rootAddr } },
    ]);
    check(block.status(rsp.tx_hash) === "ok", "gym.init ok");
}
async function getAbonement(): Promise<number> {
    const r = await mtjs.get_contract_data(node1, "gym", "abonement" + rootAddr, 3);
    if (!r.found) return -1;
    const v = JSON.parse(r.value);
    return Number(v.visits);
}

// подписаться на :node2 (next block) ДО submit; вернуть promise block (или null при таймауте)
function getNextBlock(nodeaddr: string, to: number): Promise<Block> {
    return mtjs.get_block(nodeaddr, to);
}

async function main() {
    console.log("=== GYM 2NODES (n1 " + node1 + ", n2 " + node2 + ") ===");

    await ensureRootFunded(50_000_000n);

    let src = "";
    try { src = await mtjs.fs.readFile(GYM_SRC_PATH); }
    catch (e: any) { check(false, "read gym src: " + errStr(e)); return; }
    check(src.length > 0, "gym src read (" + src.length + " bytes)");

    await ensureGym(src);
    await ensureInit();

    // подготовка: visits >= 10
    {
        const v = await getAbonement();
        if (v < 0) {
            const { rsp, block } = await submitAndWait(rootSk!, rootAddr, [
                { contract: "gym", method: "issuePass", params: { visits: 10 } },
            ]);
            check(block.status(rsp.tx_hash) === "ok", "issuePass 10 ok");
        } else if (v < 10) {
            const { rsp, block } = await submitAndWait(rootSk!, rootAddr, [
                { contract: "gym", method: "issuePass", params: { visits: 10 - v } },
            ]);
            check(block.status(rsp.tx_hash) === "ok", "issuePass top-up ok");
        } else {
            check(true, "abonement.visits >= 10 (skip issuePass)");
        }
    }

    // t1: ok TX (issuePass 1) — block_hash same на :2301/:2302
    {
        console.log("\n[t1] ok TX (issuePass 1): block_hash same on :2301/:2302");
        // подписка на :2302 (next block) ДО submit
        const p2 = getNextBlock(node2, 10);
        const { rsp, block } = await submitAndWait(rootSk!, rootAddr, [
            { contract: "gym", method: "issuePass", params: { visits: 1 } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "issuePass 1 ok on :2301");
        const bh1 = block.block_hash;
        console.log("  :2301 block_hash=" + bh1.slice(0,16));
        try {
            const b2 = await p2;
            console.log("  :2302 block_hash=" + b2.block_hash.slice(0,16));
            check(b2.block_hash === bh1, ":2302 block_hash == :2301 block_hash");
        } catch (e: any) {
            check(false, "get_block :2302 (t1): " + errStr(e));
        }
    }

    // t2: err TX (createEntry no visits) — block_hash same на :2301/:2302 (root hash сошёлся)
    {
        console.log("\n[t2] err TX (createEntry no visits): block_hash same on :2301/:2302");
        const visits0 = await getAbonement();
        const M = visits0 + 1;
        // подписка на :2302 (next block) ДО submit
        const p2 = getNextBlock(node2, 10);
        const { rsp, block } = await submitAndWaitErr(rootSk!, rootAddr, [
            { contract: "gym", method: "createEntry", params: { M: M, ttl: 300 } },
        ]);
        check(block.status(rsp.tx_hash) === "err", "createEntry M>visits err on :2301");
        const bh1 = block.block_hash;
        console.log("  :2301 block_hash=" + bh1.slice(0,16));
        try {
            const b2 = await p2;
            console.log("  :2302 block_hash=" + b2.block_hash.slice(0,16));
            check(b2.block_hash === bh1, ":2302 block_hash == :2301 block_hash (err TX deterministic)");
        } catch (e: any) {
            check(false, "get_block :2302 (t2): " + errStr(e));
        }
    }

    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0) console.log(GREEN + "DONE" + RESET);
    else            console.log(RED   + "FAIL" + RESET);
}

main().catch((e: any) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => {}, 1000000);
export {};
