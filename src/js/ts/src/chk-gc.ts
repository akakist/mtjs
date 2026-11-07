#!/usr/local/bin/mtjs
import * as std from "std";

// chk-gc.ts — GC: scan_expired → root.gc({items}) → проверка
// node :2301. root — emitter/sender.

const node = "127.0.0.1:2301";
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
async function getNonceBal(addr: string): Promise<{ nonce: bigint; bal: bigint }> {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    return { nonce: BigInt(ui.nonce), bal: BigInt(ui.balance) };
}

async function submitAndWait(sk: string, addr: string, tx: any[], value = 0, gasLimit = GAS_LIMIT, retries = 5) {
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

async function ensureRootFunded(minBalance: bigint) {
    const { bal } = await getNonceBal(rootAddr);
    console.log("  root balance:", bal.toString(), "min:", minBalance.toString());
    if (bal >= minBalance) return;
    if (bal < 200000n) { check(false, "root too low for mint"); return; }
    const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
        { contract: "root", method: "mint", params: { amount: "100000000" } },
    ], 0, 20000);
    check(block.status(rsp.tx_hash) === "ok", "mint root 100M ok");
}

function childEmits(block: Block, txHash: string): { [seq: string]: any[] } {
    const ch = block.children(txHash) as any;
    const out: { [seq: string]: any[] } = {};
    if (ch) for (const k of Object.keys(ch)) out[k] = ch[k].emits;
    return out;
}
function emitVal(e: any): any {
    // e = [name, value]; value может быть объект или JSON-строка
    let v = e[1];
    if (typeof v === "string") { try { v = JSON.parse(v); } catch (_) { return null; } }
    return v;
}
function sumEmitCount(em: { [seq: string]: any[] }, name: string): number {
    let n = 0;
    for (const k of Object.keys(em)) {
        for (const e of em[k]) {
            if (e[0] !== name) continue;
            const v = emitVal(e);
            if (v && typeof v.count === "number") n += v.count;
        }
    }
    return n;
}
function sumGcApplied(em: { [seq: string]: any[] }): number {
    let n = 0;
    for (const k of Object.keys(em)) {
        for (const e of em[k]) {
            if (e[0] !== "gc") continue;
            const v = emitVal(e);
            if (v && typeof v.applied === "number") n += v.applied;
        }
    }
    return n;
}

interface GcTotal { applied: number; granule: number; contract: number; node: number; address: number; }

async function gcPass(label: string): Promise<GcTotal> {
    let from = "";
    let total: GcTotal = { applied: 0, granule: 0, contract: 0, node: 0, address: 0 };
    let loops = 0;
    while (loops < 20) {
        loops++;
        const r = await mtjs.scan_expired(node, from, 5);
        if (r.dbids.length > 0) {
            const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
                { contract: "root", method: "gc", params: { items: r.dbids } },
            ]);
            check(block.status(rsp.tx_hash) === "ok", label + " gc ok (errstr=" + block.errstr(rsp.tx_hash) + ")");
            const em = childEmits(block, rsp.tx_hash);
            console.log("  " + label + " children:", JSON.stringify(em));
            total.applied  += sumGcApplied(em);
            total.granule  += sumEmitCount(em, "gc_granule");
            total.contract += sumEmitCount(em, "gc_contract");
            total.node     += sumEmitCount(em, "gc_node");
            total.address  += sumEmitCount(em, "gc_address");
        }
        if (r.done) break;
        from = r.next_from;
    }
    return total;
}

async function gcUntil(checkFn: () => Promise<boolean>, label: string, maxLoops = 10): Promise<GcTotal> {
    let total: GcTotal = { applied: 0, granule: 0, contract: 0, node: 0, address: 0 };
    for (let i = 0; i < maxLoops; i++) {
        const n = await gcPass(label + " #" + (i+1));
        total.applied  += n.applied;
        total.granule  += n.granule;
        total.contract += n.contract;
        total.node     += n.node;
        total.address  += n.address;
        console.log("  " + label + " #" + (i+1) + " applied=" + n.applied + " granule=" + n.granule);
        if (await checkFn()) return total;
        console.log("  " + label + " waiting 2s for block_timestamp advance...");
        await new Promise(r => setTimeout(r, 2000));
    }
    return total;
}

async function main() {
    await loadKeys();
    console.log("=== GC TEST (node " + node + ") ===");

    await ensureRootFunded(50_000_000n);

    // ============================================================
    // [t1] scan_expired — базовый вызов (типы полей)
    // ============================================================
    {
        console.log("\n[t1] scan_expired (from='') — базовый вызов");
        const r = await mtjs.scan_expired(node, "", 5);
        console.log("  scanned:", r.scanned, "dbids:", r.dbids.length, "done:", r.done);
        check(typeof r.scanned === "number", "scanned is number");
        check(Array.isArray(r.dbids), "dbids is array");
        check(typeof r.done === "boolean", "done is boolean");
        check(typeof r.next_from === "string", "next_from is string");
    }

    // ============================================================
    // [t2] createGranule ttl=2 → gcUntil x found=false
    // ============================================================
    {
        console.log("\n[t2] createGranule ttl=2 → gcUntil x deleted");
        const TTL_SRC = "/home/bsr/mtjs/src/js/tsi/dist/contracts/ttl_test.js";
        let src = "";
        try { src = await mtjs.fs.readFile(TTL_SRC); }
        catch (e: any) { check(false, "read ttl_test src: " + errStr(e)); return; }
        check(src.length > 0, "ttl_test src read");
        const c = await mtjs.get_contract(node, "ttl_test", 3);
        const method = c.found ? "contract_update" : "contract_deploy";
        {
            const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
                { contract: "root", method, params: { name: "ttl_test", src, ttl: 86400 } },
            ]);
            check(block.status(rsp.tx_hash) === "ok", "ttl_test " + method + " ok");
        }
        // cleanup x
        {
            const g = await mtjs.get_contract_data(node, "ttl_test", "x", 3);
            if (g.found) {
                await submitAndWait(rootSk, rootAddr, [
                    { contract: "ttl_test", method: "deleteX", params: {} },
                ]);
            }
        }
        // createX ttl=2
        {
            const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
                { contract: "ttl_test", method: "createX", params: { ttl: 2, v: 777 } },
            ]);
            check(block.status(rsp.tx_hash) === "ok", "createX ttl=2 ok (errstr=" + block.errstr(rsp.tx_hash) + ")");
            const g = await mtjs.get_contract_data(node, "ttl_test", "x", 3);
            check(g.found === true, "x found");
            if (g.found) {
                const v = JSON.parse(g.value);
                check(v.v === 777, "x.v == 777 (got " + v.v + ")");
            }
        }
        const t2 = await gcUntil(async () => {
            const g = await mtjs.get_contract_data(node, "ttl_test", "x", 3);
            return g.found === false;
        }, "[t2]");
        console.log("  applied_total:", t2.applied, "granule:", t2.granule);
        check(t2.applied > 0, "gc applied > 0 (got " + t2.applied + ")");
        check(t2.granule > 0, "gc_granule > 0 (got " + t2.granule + ")");
        await new Promise(r => setTimeout(r, 300));
        const g2 = await mtjs.get_contract_data(node, "ttl_test", "x", 3);
        check(g2.found === false, "x deleted (found=false)");
    }

    // ============================================================
    // [t3] y ttl=2 expired, z ttl=100000 живёт
    // ============================================================
    {
        console.log("\n[t3] y ttl=2 (expired), z ttl=100000 (живёт)");
        for (const k of ["y", "z"]) {
            const g = await mtjs.get_contract_data(node, "ttl_test", k, 3);
            if (g.found) {
                const m = "delete" + k.toUpperCase();
                await submitAndWait(rootSk, rootAddr, [
                    { contract: "ttl_test", method: m, params: {} },
                ]);
            }
        }
        {
            const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
                { contract: "ttl_test", method: "createY", params: { ttl: 2 } },
            ]);
            check(block.status(rsp.tx_hash) === "ok", "createY ttl=2 ok");
        }
        {
            const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
                { contract: "ttl_test", method: "createZ", params: { ttl: 100000 } },
            ]);
            check(block.status(rsp.tx_hash) === "ok", "createZ ttl=100000 ok");
        }
        const t3 = await gcUntil(async () => {
            const gy = await mtjs.get_contract_data(node, "ttl_test", "y", 3);
            return gy.found === false;
        }, "[t3]");
        console.log("  applied_total:", t3.applied, "granule:", t3.granule);
        check(t3.granule >= 1, "gc_granule >= 1 (y, got " + t3.granule + ")");
        await new Promise(r => setTimeout(r, 300));
        const gy = await mtjs.get_contract_data(node, "ttl_test", "y", 3);
        const gz = await mtjs.get_contract_data(node, "ttl_test", "z", 3);
        check(gy.found === false, "y deleted (ttl=2, found=false)");
        check(gz.found === true, "z NOT deleted (ttl=100000, found=true)");
        {
            const g = await mtjs.get_contract_data(node, "ttl_test", "z", 3);
            if (g.found) {
                await submitAndWait(rootSk, rootAddr, [
                    { contract: "ttl_test", method: "deleteZ", params: {} },
                ]);
            }
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

