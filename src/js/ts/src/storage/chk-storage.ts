#!/usr/local/bin/mtjs
import * as std from "std";

// chk-storage.ts — JSI_Granule (create_granule/get_granule/set/get/del/touch).
// node :2301. root — sender.

const node = "127.0.0.1:2301";
const STORAGE_SRC_PATH = "/home/bsr/mtjs/src/js/tsi/dist/contracts/storage_test.js";
const GAS_LIMIT = 100000;
const KEYS_PATH = std.getenv("MTJS_KEYS_PATH") || "keys_test.json";

let rootSk = ""; let rootPk = "";
async function loadKeys() {
    const raw = await mtjs.fs.readFile(KEYS_PATH);
    const keys = JSON.parse(raw);
    const r = keys.users?.root?.keys?.[0];
    if (!r?.sk || !r?.pk) throw new Error("keys_test.json: no users.root.keys[0].sk/pk");
    rootSk = r.sk; rootPk = r.pk;
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
let lastBlock: Block | null = null;
mtjs.tx_subscribe(node, (b: Block) => {
    lastBlock = b;
    for (const h of b.tx_hashes) {
        const p = pending[h];
        if (p) { delete pending[h]; clearTimeout(p.timer); p.resolve(b); }
    }
});
function waitTx(h: string, to: number): Promise<Block> {
    if (lastBlock && lastBlock.tx_hashes.indexOf(h) >= 0) return Promise.resolve(lastBlock);
    return new Promise<Block>((res, rej) => {
        const timer = setTimeout(() => { delete pending[h]; rej({errcode:1,errstr:"timeout"}); }, to*1000);
        pending[h] = { resolve: res, reject: rej, timer };
    });
}

async function getNonce(addr: string): Promise<bigint> {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    return BigInt(ui.nonce);
}
async function submitAndWait(sk: string, addr: string, tx: any[], value = 0, gasLimit = GAS_LIMIT, retries = 5) {
    for (let a = 0; a < retries; a++) {
        const nonce = await getNonce(addr);
        const req = { tx, nonce: nonce.toString(), value, gasLimit, priority: 0, sender: addr, key_id: 0 };
        const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, nonce.toString(), () => {});
        if (rsp.err) { if (a+1 < retries) { await new Promise(r=>setTimeout(r,300)); continue; } throw new Error("submit: "+rsp.err_str); }
        const b = await waitTx(rsp.tx_hash, 10);
        const st = b.status(rsp.tx_hash);
        const es = b.errstr(rsp.tx_hash) || "";
        if (st === "err" && es.indexOf("invalid nonce") >= 0) { if (a+1 < retries) { await new Promise(r=>setTimeout(r,300)); continue; } throw new Error("invalid nonce: "+es); }
        return { rsp, block: b, nonce };
    }
    throw new Error("submitAndWait: retries exhausted");
}
async function submitAndWaitErr(sk: string, addr: string, tx: any[], value = 0, gasLimit = GAS_LIMIT, retries = 5) {
    for (let a = 0; a < retries; a++) {
        const nonce = await getNonce(addr);
        const req = { tx, nonce: nonce.toString(), value, gasLimit, priority: 0, sender: addr, key_id: 0 };
        const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, nonce.toString(), () => {});
        if (rsp.err) { if (a+1 < retries) { await new Promise(r=>setTimeout(r,300)); continue; } throw new Error("submit: "+rsp.err_str); }
        let b: Block;
        try { b = await waitTx(rsp.tx_hash, 30); } catch(e) { if (a+1 < retries) { await new Promise(r=>setTimeout(r,300)); continue; } throw e; }
        const st = b.status(rsp.tx_hash);
        const es = b.errstr(rsp.tx_hash) || "";
        if (st === "err" && es.indexOf("invalid nonce") >= 0) { if (a+1 < retries) { await new Promise(r=>setTimeout(r,300)); continue; } throw new Error("invalid nonce: "+es); }
        return { rsp, block: b, nonce };
    }
    throw new Error("submitAndWaitErr: retries exhausted");
}

const rootAddr = mtjs.addr_from_name("root");

async function ensureRootFunded(minBalance: bigint) {
    const ui = await mtjs.get_user_nonce(node, rootAddr, 3);
    const bal = BigInt(ui.balance);
    console.log("  root balance:", bal.toString(), "min:", minBalance.toString());
    if (bal >= minBalance) { check(true, "root funded >= " + minBalance.toString()); return; }
    if (bal < 200000n) { check(false, "root too low for mint — clean db"); return; }
    const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
        { contract: "root", method: "mint", params: { amount: "100000000" } },
    ], 0, 20000);
    check(block.status(rsp.tx_hash) === "ok", "mint root 100M ok");
}

async function ensureStorageTest(src: string) {
    const c = await mtjs.get_contract(node, "storage_test", 3);
    const method = c.found ? "contract_update" : "contract_deploy";
    const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
        { contract: "root", method, params: { name: "storage_test", src, ttl: 86400 } },
    ]);
    check(block.status(rsp.tx_hash) === "ok", "storage_test " + method + " ok (errstr=" + (block.errstr(rsp.tx_hash)||"") + ")");
}

async function main() {
    await loadKeys();
    console.log("=== STORAGE TEST (node " + node + ") ===");

    await ensureRootFunded(50_000_000n);

    let src = "";
    try { src = await mtjs.fs.readFile(STORAGE_SRC_PATH); }
    catch (e: any) { check(false, "read src: " + errStr(e)); return; }
    check(src.length > 0, "storage_test src read (" + src.length + " bytes)");

    await ensureStorageTest(src);

    {
        console.log("\n[t1] create → set → get");
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "storage_test", method: "t1_create_set_get", params: {} },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "t1 ok (errstr=" + (block.errstr(rsp.tx_hash)||"") + ")");
        // проверяем гранулу (container = {"hello":"world"})
        const r = await mtjs.get_contract_data(node, "storage_test", "t1/a", 3);
        check(r.found === true, "t1/a found");
        if (r.found) {
            const v = JSON.parse(r.value);
            check(v.hello === "world", "t1/a.hello == 'world' (got " + v.hello + ")");
        }
    }

    {
        console.log("\n[t2] create → set → del → get (null)");
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "storage_test", method: "t2_del", params: {} },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "t2 ok (errstr=" + (block.errstr(rsp.tx_hash)||"") + ")");
        const r = await mtjs.get_contract_data(node, "storage_test", "t2/x", 3);
        check(r.found === false, "t2/x deleted (found=false)");
    }

    {
        console.log("\n[t3] create (nested) → set → get");
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "storage_test", method: "t3_nested", params: {} },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "t3 ok (errstr=" + (block.errstr(rsp.tx_hash)||"") + ")");
        const r = await mtjs.get_contract_data(node, "storage_test", "t3/a/b", 3);
        check(r.found === true, "t3/a/b found");
        if (r.found) {
            const v = JSON.parse(r.value);
            check(v.n === 42, "t3/a/b.n == 42 (got " + v.n + ")");
        }
    }

    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0) console.log(GREEN + "DONE" + RESET);
    else            console.log(RED   + "FAIL" + RESET);
    std.exit(fail);
}

main().catch((e: any) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => {}, 1000000);
export {};
