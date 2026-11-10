#!/usr/local/bin/mtjs
import * as std from "std";
// chk-gc-address.ts — GC адреса: ttl expired → удалить адрес, сжечь баланс.
// node :2301. root — sender.
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
    const ui = await mtjs.get_user_nonce(node, rootAddr, 3);
    if (BigInt(ui.balance) >= minBalance)
        return;
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
    check(block.status(rsp.tx_hash) === "ok", label + " gc ok");
    let applied = 0, address = 0;
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
                if (e[0] === "gc_address" && v && typeof v.count === "number")
                    address += v.count;
            }
        }
    console.log("  [" + label + "] applied=" + applied + " address=" + address);
    return { applied, address };
}
async function main() {
    await loadKeys();
    console.log("=== GC ADDRESS (node " + node + ") ===");
    await ensureRootFunded(50000000n);
    // 1. Создать новый адрес с ttl=2 (blake2b от случайного ника)
    const nick = "gc_addr_" + Date.now();
    const newAddr = mtjs.addr_from_name(nick);
    const newPk = mtjs.blake2b_hex(nick + "_pk");
    {
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "root", method: "create_address", params: {
                    addr: newAddr,
                    keys: [{ type: 0, keybuffer: newPk }],
                    ttl: 2,
                } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "create_address ok (errstr=" + (block.errstr(rsp.tx_hash) || "") + ")");
    }
    // 2. Пополнить адрес (transfer 1000 от root)
    {
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "root", method: "transfer", params: { to: newAddr, amount: "1000" } },
        ], 1000);
        check(block.status(rsp.tx_hash) === "ok", "transfer ok");
    }
    const info0 = await mtjs.get_address_info(node, newAddr, 3);
    check(info0.found === true, "address found (before gc)");
    check(BigInt(info0.balance) === 1000n, "balance=1000 (got " + info0.balance + ")");
    // 3. Ждать 3s
    await new Promise(r => setTimeout(r, 3000));
    // 4. scan + gc
    const scan = await scanOnce("addr scan");
    check(scan.dbids.length > 0, "scan dbids > 0 (got " + scan.dbids.length + ")");
    if (scan.dbids.length > 0) {
        const g = await gcOnce("addr gc", scan.dbids);
        check(g.applied > 0, "gc applied > 0");
        check(g.address > 0, "gc_address > 0 (got " + g.address + ")");
    }
    // 5. Проверить, что адрес удалён
    await new Promise(r => setTimeout(r, 2000));
    const info1 = await mtjs.get_address_info(node, newAddr, 3);
    check(info1.found === false, "address deleted (found=false)");
    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0)
        console.log(GREEN + "DONE" + RESET);
    else
        console.log(RED + "FAIL" + RESET);
    std.exit(fail);
}
main().catch((e) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => { }, 1000000);
