#!/usr/local/bin/mtjs
import * as std from "std";
// chk-gc-node.ts — GC ноды: ttl expired + disabled → вернуть stakes, удалить из bc_nodelist, удалить bc_node.
// enabled нода НЕ GC-шится (fix_missed_rounds обновляет activity.update_time).
// node :2301.
const node = "127.0.0.1:2301";
const KEYS_PATH = std.getenv("MTJS_KEYS_PATH") || "keys_test.json";
const GAS_LIMIT = 100000;
let rootSk = "";
let rootPk = "";
async function loadKeys() {
    const raw = await mtjs.fs.readFile(KEYS_PATH);
    const keys = JSON.parse(raw);
    const r = keys.users?.root?.keys?.[0];
    if (!r?.sk || !r?.pk)
        throw new Error("keys_test.json: no users.root.keys[0]");
    rootSk = r.sk;
    rootPk = r.pk;
}
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
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    return BigInt(ui.nonce);
}
async function getNonceBal(addr) {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    return { nonce: BigInt(ui.nonce), bal: BigInt(ui.balance) };
}
async function submitAndWait(sk, addr, tx, value = 0, gasLimit = GAS_LIMIT, retries = 5) {
    for (let a = 0; a < retries; a++) {
        const nonce = await getNonce(addr);
        const req = { tx, nonce: nonce.toString(), value, gasLimit, priority: 0, sender: addr, key_id: 0 };
        const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, nonce.toString(), () => { });
        if (rsp.err) {
            if (a + 1 < retries) {
                await new Promise(r => setTimeout(r, 300));
                continue;
            }
            throw new Error("submit: " + rsp.err_str);
        }
        const b = await waitTx(rsp.tx_hash, 10);
        const st = b.status(rsp.tx_hash);
        const es = b.errstr(rsp.tx_hash) || "";
        if (st === "err" && es.indexOf("invalid nonce") >= 0) {
            if (a + 1 < retries) {
                await new Promise(r => setTimeout(r, 300));
                continue;
            }
            throw new Error("invalid nonce: " + es);
        }
        return { rsp, block: b, nonce };
    }
    throw new Error("submitAndWait: retries exhausted");
}
const rootAddr = mtjs.addr_from_name("root");
async function ensureRootFunded(minBalance) {
    const { bal } = await getNonceBal(rootAddr);
    if (bal >= minBalance)
        return;
    if (bal < 200000n) {
        check(false, "root too low");
        return;
    }
    await submitAndWait(rootSk, rootAddr, [
        { contract: "root", method: "mint", params: { amount: "100000000" } },
    ], 0, 20000);
}
async function scanOnce(label) {
    const r = await mtjs.scan_expired(node, "", 5);
    console.log("  [" + label + "] scanned=" + r.scanned + " dbids=" + r.dbids.length + " done=" + r.done);
    return r;
}
async function gcOnce(label, dbids) {
    const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
        { contract: "root", method: "gc", params: { items: dbids } },
    ]);
    check(block.status(rsp.tx_hash) === "ok", label + " gc ok (errstr=" + (block.errstr(rsp.tx_hash) || "") + ")");
    let applied = 0, granule = 0, contract = 0, node_ = 0, address = 0;
    const em = block.children(rsp.tx_hash);
    if (em)
        for (const k of Object.keys(em)) {
            for (const e of em[k].emits) {
                let v = e[1];
                if (typeof v === "string")
                    try {
                        v = JSON.parse(v);
                    }
                    catch (_) { }
                if (e[0] === "gc" && v && typeof v.applied === "number")
                    applied += v.applied;
                if (e[0] === "gc_granule" && v && typeof v.count === "number")
                    granule += v.count;
                if (e[0] === "gc_contract" && v && typeof v.count === "number")
                    contract += v.count;
                if (e[0] === "gc_node" && v && typeof v.count === "number")
                    node_ += v.count;
                if (e[0] === "gc_address" && v && typeof v.count === "number")
                    address += v.count;
            }
        }
    console.log("  [" + label + "] applied=" + applied + " granule=" + granule + " contract=" + contract + " node=" + node_ + " address=" + address);
    return { applied, granule, contract, node: node_, address };
}
async function main() {
    await loadKeys();
    console.log("=== GC NODE (node " + node + ") ===");
    await ensureRootFunded(50000000n);
    // ============================================================
    // [t1] node_create с ttl=2
    // ============================================================
    console.log("\n[t1] node_create с ttl=2 (BLS + ed25519)");
    const kpB = mtjs.generate_keypair(3);
    const kpE = mtjs.generate_keypair(0);
    const nodeName = "n_gc_test_" + Date.now();
    {
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "root", method: "node_create", params: {
                    name: nodeName,
                    ip: "127.0.0.1:9999",
                    pk_ed: kpE.pk,
                    pk_bls: kpB.pk,
                    ttl: 2,
                } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "node_create ok (errstr=" + (block.errstr(rsp.tx_hash) || "") + ")");
    }
    {
        const ni = await mtjs.get_node_info(node, nodeName, 3);
        check(ni.found === true, "node found (before gc)");
        check(ni.enabled === true, "node enabled");
    }
    // ============================================================
    // [t2] node_stake 100 от root
    // ============================================================
    console.log("\n[t2] node_stake 100 от root");
    {
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "root", method: "node_stake", params: { node: nodeName, amount: "100" } },
        ], 100);
        check(block.status(rsp.tx_hash) === "ok", "node_stake ok (errstr=" + (block.errstr(rsp.tx_hash) || "") + ")");
    }
    const rootAfterStake = await getNonceBal(rootAddr);
    console.log("  root balance after stake:", rootAfterStake.bal.toString());
    // ============================================================
    // [t2.1] node_disable — чтобы GC сработал (enabled не GC-шится)
    // ============================================================
    console.log("\n[t2.1] node_disable (чтобы GC сработал)");
    {
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "root", method: "node_disable", params: { node: nodeName } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "node_disable ok (errstr=" + (block.errstr(rsp.tx_hash) || "") + ")");
    }
    {
        const ni = await mtjs.get_node_info(node, nodeName, 3);
        check(ni.found === true, "node found (disabled)");
        check(ni.enabled === false, "node disabled");
    }
    // ============================================================
    // [t3] wait 3s (ttl=2 expired) → scan → gc
    // ============================================================
    console.log("\n[t3] wait 3s → scan → gc");
    await new Promise(r => setTimeout(r, 3000));
    const scan = await scanOnce("node scan");
    check(scan.dbids.length > 0, "scan dbids > 0 (got " + scan.dbids.length + ")");
    if (scan.dbids.length > 0) {
        const g = await gcOnce("node gc", scan.dbids);
        check(g.applied > 0, "gc applied > 0 (got " + g.applied + ")");
        check(g.node > 0, "gc_node > 0 (got " + g.node + ")");
    }
    // ============================================================
    // [t4] проверка после gc
    // ============================================================
    console.log("\n[t4] проверка после gc");
    await new Promise(r => setTimeout(r, 2000));
    {
        const ni = await mtjs.get_node_info(node, nodeName, 3);
        check(ni.found === false, "node deleted (found=false)");
    }
    {
        const rootAfterGc = await getNonceBal(rootAddr);
        console.log("  root balance after gc:", rootAfterGc.bal.toString());
        check(rootAfterGc.bal >= rootAfterStake.bal - 10000n, "stake returned (root bal " + rootAfterStake.bal.toString() + " -> " + rootAfterGc.bal.toString() + ")");
    }
    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0)
        console.log(GREEN + "DONE" + RESET);
    else
        console.log(RED + "FAIL" + RESET);
    std.exit(fail);
}
main().catch((e) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => { }, 1000000);
