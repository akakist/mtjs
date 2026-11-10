#!/usr/local/bin/mtjs
import * as std from "std";

// chk-gc-contract.ts — GC контракта: ttl expired → удалить контракт+balance, owners пусты.
// node :2301. root — sender.

const node = "127.0.0.1:2301";
const KEYS_PATH = std.getenv("MTJS_KEYS_PATH") || "keys_test.json";
const GAS_LIMIT = 100000;

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
function check(cond: boolean, msg: string) {
    if (cond) { pass++; console.log("  " + GREEN + "✅" + RESET + " " + msg); }
    else      { fail++; console.log("  " + RED   + "❌" + RESET + " " + msg); }
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

const rootAddr = mtjs.addr_from_name("root");

async function ensureRootFunded(minBalance: bigint) {
    const ui = await mtjs.get_user_nonce(node, rootAddr, 3);
    if (BigInt(ui.balance) >= minBalance) return;
    await submitAndWait(rootSk, rootAddr, [
        { contract: "root", method: "mint", params: { amount: "100000000" } },
    ], 0, 20000);
}

async function scanOnce(label: string) {
    const r = await mtjs.scan_expired(node, "", 5);
    console.log("  [" + label + "] scanned=" + r.scanned + " dbids=" + r.dbids.length + " done=" + r.done);
    return r;
}

async function gcOnce(label: string, dbids: string[]) {
    const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
        { contract: "root", method: "gc", params: { items: dbids } },
    ]);
    check(block.status(rsp.tx_hash) === "ok", label + " gc ok");
    let applied = 0, contract = 0;
    const em = block.children(rsp.tx_hash) as any;
    if (em) for (const k of Object.keys(em)) {
        for (const e of em[k].emits) {
            let v = e[1]; if (typeof v === "string") try { v = JSON.parse(v); } catch(_) {}
            if (e[0] === "gc" && v && typeof v.applied === "number") applied += v.applied;
            if (e[0] === "gc_contract" && v && typeof v.count === "number") contract += v.count;
        }
    }
    console.log("  [" + label + "] applied=" + applied + " contract=" + contract);
    return { applied, contract };
}

async function main() {
    await loadKeys();
    console.log("=== GC CONTRACT (node " + node + ") ===");
    await ensureRootFunded(50_000_000n);

    // 1. Deploy контракта с ttl=2
    const contractName = "gc_test_contract_" + Date.now();
    const src = "registerMethod('noop', function(p, m) {});";

    {
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "root", method: "contract_deploy", params: {
                name: contractName, src, ttl: 2,
            } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "contract_deploy ok (errstr=" + (block.errstr(rsp.tx_hash)||"") + ")");
    }

    const c0 = await mtjs.get_contract(node, contractName, 3);
    check(c0.found === true, "contract found (before gc)");
    check(c0.ttl === 2, "contract ttl=2 (got " + c0.ttl + ")");

    // 2. Создать гранулу (activity для контракта обновится)
    {
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: contractName, method: "noop", params: {} },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "noop ok");
    }

    // 3. Ждать 3s (ttl=2 expired)
    await new Promise(r => setTimeout(r, 3000));

    // 4. scan + gc
    const scan = await scanOnce("contract scan");
    check(scan.dbids.length > 0, "scan dbids > 0 (got " + scan.dbids.length + ")");
    if (scan.dbids.length > 0) {
        const g = await gcOnce("contract gc", scan.dbids);
        check(g.applied > 0, "gc applied > 0");
        check(g.contract > 0, "gc_contract > 0 (got " + g.contract + ")");
    }

    // 5. Проверить, что контракт удалён
    await new Promise(r => setTimeout(r, 2000));
    const c1 = await mtjs.get_contract(node, contractName, 3);
    check(c1.found === false, "contract deleted (found=false)");

    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0) console.log(GREEN + "DONE" + RESET);
    else            console.log(RED   + "FAIL" + RESET);
    std.exit(fail);
}

main().catch((e: any) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => {}, 1000000);
export {};
