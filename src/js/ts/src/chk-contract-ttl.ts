#!/usr/local/bin/mtjs
import * as std from "std";

const node = "127.0.0.1:2301";
const KEYS_PATH = std.getenv("MTJS_KEYS_PATH") || "keys_test.json";

let rootSk = ""; let rootAddr = "";
async function loadKeys() {
    const raw = await mtjs.fs.readFile(KEYS_PATH);
    const keys = JSON.parse(raw);
    const r = keys.users?.root?.keys?.[0];
    if (!r?.sk || !r?.pk) throw new Error("keys_test.json: no users.root.keys[0]");
    rootSk = r.sk; rootAddr = mtjs.addr_from_name("root");
}

const GREEN = "\x1b[32m"; const RED = "\x1b[31m"; const RESET = "\x1b[0m";
let pass = 0, fail = 0;
function check(c: boolean, m: string) {
    if (c) { pass++; console.log("  " + GREEN + "✅" + RESET + " " + m); }
    else   { fail++; console.log("  " + RED   + "❌" + RESET + " " + m); }
}
function errStr(e: any): string {
    return (e && typeof e === "object" && ("errcode" in e || "errstr" in e))
        ? "errcode=" + e.errcode + " errstr=" + e.errstr : String(e);
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
        const timer = setTimeout(() => { delete pending[h]; rej({errcode:1, errstr:"timeout"}); }, to * 1000);
        pending[h] = { resolve: res, reject: rej, timer };
    });
}
async function getNonce(addr: string): Promise<bigint> {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    return BigInt(ui.nonce);
}
async function submitAndWait(tx: any[], value = 0, gasLimit = 100000, retries = 5) {
    for (let a = 0; a < retries; a++) {
        const nonce = await getNonce(rootAddr);
        const req = { tx, nonce: nonce.toString(), value, gasLimit, priority: 0, sender: rootAddr, key_id: 0 };
        const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), rootSk, nonce.toString(), () => {});
        if (rsp.err) { if (a+1 < retries) { await new Promise(r=>setTimeout(r,300)); continue; } throw new Error(rsp.err_str); }
        const b = await waitTx(rsp.tx_hash, 10);
        return { rsp, block: b };
    }
    throw new Error("retries exhausted");
}
async function submitAndWaitErr(tx: any[], value = 0, gasLimit = 100000, retries = 3) {
    for (let a = 0; a < retries; a++) {
        const nonce = await getNonce(rootAddr);
        const req = { tx, nonce: nonce.toString(), value, gasLimit, priority: 0, sender: rootAddr, key_id: 0 };
        const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), rootSk, nonce.toString(), () => {});
        if (rsp.err) { if (a+1 < retries) { await new Promise(r=>setTimeout(r,300)); continue; } throw new Error(rsp.err_str); }
        const b = await waitTx(rsp.tx_hash, 10);
        return { rsp, block: b };
    }
    throw new Error("retries exhausted");
}

const NAME = "ttl_deploy_" + Date.now();
const SRC = "registerMethod('noop', function(p, m) {});";

async function main() {
    await loadKeys();
    console.log("=== CONTRACT TTL (node " + node + ", contract " + NAME + ") ===");

    // t1: deploy с ttl=2
    {
        console.log("\n[t1] contract_deploy ttl=2");
        const { rsp, block } = await submitAndWait([
            { contract: "root", method: "contract_deploy", params: { name: NAME, src: SRC, ttl: 2 } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "deploy ok (errstr=" + (block.errstr(rsp.tx_hash)||"") + ")");
        const c = await mtjs.get_contract(node, NAME, 3);
        console.log("  get_contract:", JSON.stringify({found: c.found, ttl: c.ttl, update_time: c.update_time}));
        check(c.found === true, "found");
        check(c.ttl === 2, "ttl == 2 (got " + c.ttl + ")");
        check(c.update_time > 0, "update_time > 0");
    }

    // t2: contract_update → update_time растёт, ttl тот же
    {
        console.log("\n[t2] contract_update → update_time растёт, ttl=2");
        const c0 = await mtjs.get_contract(node, NAME, 3);
        await new Promise(r => setTimeout(r, 1200));   // чтобы update_time сдвинулся
        const { rsp, block } = await submitAndWait([
            { contract: "root", method: "contract_update", params: { name: NAME, src: SRC + "\n//upd" } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "update ok (errstr=" + (block.errstr(rsp.tx_hash)||"") + ")");
        const c1 = await mtjs.get_contract(node, NAME, 3);
        console.log("  update_time:", c0.update_time, "->", c1.update_time, "ttl:", c1.ttl);
        check(c1.ttl === 2, "ttl == 2 (не изменился)");
        check(c1.update_time > c0.update_time, "update_time вырос: " + c0.update_time + " -> " + c1.update_time);
    }

    // t3: deploy без ttl → err
    {
        console.log("\n[t3] contract_deploy без ttl → err");
        const { rsp, block } = await submitAndWaitErr([
            { contract: "root", method: "contract_deploy", params: { name: NAME + "_nottl", src: SRC } },
        ]);
        check(block.status(rsp.tx_hash) === "err", "status err");
        const es = block.errstr(rsp.tx_hash) || "";
        console.log("  errstr:", es);
        check(es.indexOf("ttl") >= 0, "errstr mentions ttl");
    }

    // t4: deploy с ttl=0 → err
    {
        console.log("\n[t4] contract_deploy ttl=0 → err");
        const { rsp, block } = await submitAndWaitErr([
            { contract: "root", method: "contract_deploy", params: { name: NAME + "_zero", src: SRC, ttl: 0 } },
        ]);
        check(block.status(rsp.tx_hash) === "err", "status err");
        const es = block.errstr(rsp.tx_hash) || "";
        console.log("  errstr:", es);
        check(es.indexOf("ttl") >= 0, "errstr mentions ttl");
    }

    // t5: deploy с ttl > MAX → err
    {
        console.log("\n[t5] contract_deploy ttl > MAX → err");
        const { rsp, block } = await submitAndWaitErr([
            { contract: "root", method: "contract_deploy", params: { name: NAME + "_max", src: SRC, ttl: 315360001 } },
        ]);
        check(block.status(rsp.tx_hash) === "err", "status err");
        const es = block.errstr(rsp.tx_hash) || "";
        console.log("  errstr:", es);
        check(es.indexOf("MAX") >= 0, "errstr mentions MAX");
    }

    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0) console.log(GREEN + "DONE" + RESET);
    else            console.log(RED   + "FAIL" + RESET);
    std.exit(fail);
}

main().catch((e: any) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => {}, 1000000);
export {};
