#!/usr/local/bin/mtjs
import * as std from "std";

const node = "127.0.0.1:2301";
const INTERRUPT_SRC_PATH = "/home/bsr/mtjs/src/js/tsi/dist/contracts/interrupt.js";
const KEYS_PATH = std.getenv("MTJS_KEYS_PATH") || "keys_test.json";

let rootSk = ""; let rootPk = "";
async function loadKeys() {
    const raw = await mtjs.fs.readFile(KEYS_PATH);
    const keys = JSON.parse(raw);
    const r = keys.users?.root?.keys?.[0];
    if (!r?.sk || !r?.pk) throw new Error("keys_test.json: no users.root.keys[0]");
    rootSk = r.sk; rootPk = r.pk;
}

const GREEN = "\x1b[32m"; const RED = "\x1b[31m"; const RESET = "\x1b[0m";
let pass = 0; let fail = 0;
function check(c: boolean, m: string) {
    if (c) { pass++; console.log("  " + GREEN + "✅" + RESET + " " + m); }
    else   { fail++; console.log("  " + RED   + "❌" + RESET + " " + m); }
}
function errStr(e: any): string {
    return (e && typeof e === "object" && ("errcode" in e || "errstr" in e))
        ? "errcode=" + e.errcode + " errstr=" + e.errstr : String(e);
}

const rootAddr = mtjs.addr_from_name("root");

async function ensureContract(name: string, srcPath: string) {
    const src = await mtjs.fs.readFile(srcPath);
    const c = await mtjs.get_contract(node, name, 3);
    const ui = await mtjs.get_user_nonce(node, rootAddr, 3);
    const nonce = ui.nonce;
    const method = c.found ? "contract_update" : "contract_deploy";
    const req = { tx: [{ contract: "root", method, params: { name, src } }],
                  nonce, value: 0, gasLimit: 100000, priority: 0, sender: rootAddr, key_id: 0 };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), rootSk, nonce, () => {});
    if (rsp.err) throw new Error(method + ": " + rsp.err_str);
    await mtjs.tx_wait_block(node, rsp.tx_hash, 10);
}

async function main() {
    await loadKeys();
    console.log("=== CALLVIEW LOOP (node " + node + ") ===");

    await ensureContract("interrupt", INTERRUPT_SRC_PATH);

    {
        console.log("\n[t1] view loopView(timeout_sec=1) → err (interrupted)");
        const t0 = Date.now();
        const r = await mtjs.call_view(node, "interrupt", "loopView", {}, 1, 5);
        const dt = Date.now() - t0;
        console.log("  result:", JSON.stringify(r), "in", dt, "ms");
        check(r.found === true, "found");
        check(r.error === true, "error=true (interrupted)");
        check(r.result.indexOf("interrupt") >= 0, "result mentions interrupt");
        check(dt < 3000, "interrupted < 3s (got " + dt + "ms)");
    }

    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0) console.log(GREEN + "DONE" + RESET);
    else            console.log(RED   + "FAIL" + RESET);
    std.exit(fail);
}

main().catch((e: any) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => {}, 1000000);
export {};
