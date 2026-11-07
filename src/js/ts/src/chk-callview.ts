#!/usr/local/bin/mtjs
import * as std from "std";

const node = "127.0.0.1:2301";
const GYM_SRC_PATH = "/home/bsr/mtjs/src/js/tsi/dist/contracts/gym.js";
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
    return BigInt(ui.nonce);
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

const rootAddr = mtjs.addr_from_name("root");

async function ensureGym(src: string) {
    const c = await mtjs.get_contract(node, "gym", 3);
    if (c.found) {
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "root", method: "contract_update", params: { name: "gym", src } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "contract_update gym ok (errstr=" + block.errstr(rsp.tx_hash) + ")");
    } else {
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "root", method: "contract_deploy", params: { name: "gym", src, ttl: 86400 } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "contract_deploy gym ok (errstr=" + block.errstr(rsp.tx_hash) + ")");
    }
}
async function ensureInit() {
    const r = await mtjs.get_contract_data(node, "gym", "admin", 3);
    if (r.found) { check(true, "gym already init (admin found)"); return; }
    const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
        { contract: "gym", method: "init", params: { admin: rootAddr } },
    ]);
    check(block.status(rsp.tx_hash) === "ok", "gym.init ok (errstr=" + block.errstr(rsp.tx_hash) + ")");
}

async function main() {
    await loadKeys();
    console.log("=== CALLVIEW (node " + node + ") ===");

    // ensure root funded
    {
        const ui = await mtjs.get_user_nonce(node, rootAddr, 3);
        if (BigInt(ui.balance) < 50000000n) {
            const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
                { contract: "root", method: "mint", params: { amount: "100000000" } },
            ], 0, 20000);
            check(block.status(rsp.tx_hash) === "ok", "mint root 100M ok (errstr=" + block.errstr(rsp.tx_hash) + ")");
        } else {
            check(true, "root funded >= 50M");
        }
    }

    // ensure gym deployed + init
    let src = "";
    try { src = await mtjs.fs.readFile(GYM_SRC_PATH); }
    catch (e: any) { check(false, "read gym src: " + errStr(e)); return; }
    check(src.length > 0, "gym src read (" + src.length + " bytes)");
    await ensureGym(src);
    await ensureInit();

    // t1: getAdmin (view) — admin == root
    {
        console.log("\n[t1] call_view gym.getAdmin");
        const r = await mtjs.call_view(node, "gym", "getAdmin", {}, 5, 5);
        console.log("  result:", JSON.stringify(r));
        check(r.found === true, "found");
        check(r.error === false, "no error");
        try {
            const v = JSON.parse(r.result);
            check(typeof v.addr === "string" && v.addr.length > 0, "addr non-empty");
            check(v.addr === rootAddr, "addr == root");
        } catch (e: any) {
            check(false, "result JSON: " + errStr(e));
        }
    }

    // t2: unknown view
    {
        console.log("\n[t2] call_view gym.unknownView");
        const r = await mtjs.call_view(node, "gym", "unknownView", {}, 5, 5);
        console.log("  result:", JSON.stringify(r));
        check(r.found === false, "found=false");
        check(r.error === true, "error=true");
        check(r.result.indexOf("view not found") >= 0, "result mentions 'view not found'");
    }

    // t3: unknown contract
    {
        console.log("\n[t3] call_view unknownContract.getAdmin");
        const r = await mtjs.call_view(node, "unknownContract", "getAdmin", {}, 5, 5);
        console.log("  result:", JSON.stringify(r));
        check(r.found === false, "found=false");
        check(r.error === true, "error=true");
        check(r.result.indexOf("contract not found") >= 0, "result mentions 'contract not found'");
    }

    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0) console.log(GREEN + "DONE" + RESET);
    else            console.log(RED   + "FAIL" + RESET);
    std.exit(fail);
}

main().catch((e: any) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => {}, 1000000);
export {};
