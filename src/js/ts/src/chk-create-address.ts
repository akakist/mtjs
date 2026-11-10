#!/usr/local/bin/mtjs
import * as std from "std";

const node = "127.0.0.1:2301";
const KEYS_PATH = std.getenv("MTJS_KEYS_PATH") || "keys_test.json";

let rootSk = ""; let rootAddr = "";

async function loadKeys() {
    const raw = await mtjs.fs.readFile(KEYS_PATH);
    const keys = JSON.parse(raw);
    const r = keys.users?.root?.keys?.[0];
    if (!r?.sk || !r?.pk) throw new Error("keys_test.json: no users.root.keys[0].sk/pk");
    rootSk = r.sk;
    rootAddr = mtjs.addr_from_name("root");
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
async function submitAndWait(sk: string, addr: string, tx: any[], value = 0, gasLimit = 100000, retries = 5) {
    for (let a = 0; a < retries; a++) {
        const nonce = await getNonce(addr);
        const req = { tx, nonce: nonce.toString(), value, gasLimit, priority: 0, sender: addr, key_id: 0 };
        const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, nonce.toString(), () => {});
        if (rsp.err) { if (a+1 < retries) { await new Promise(r=>setTimeout(r,300)); continue; } throw new Error(rsp.err_str); }
        const b = await waitTx(rsp.tx_hash, 10);
        const st = b.status(rsp.tx_hash);
        const es = b.errstr(rsp.tx_hash) || "";
        if (st === "err" && es.indexOf("invalid nonce") >= 0) { if (a+1 < retries) { await new Promise(r=>setTimeout(r,300)); continue; } throw new Error(es); }
        return { rsp, block: b };
    }
    throw new Error("retries exhausted");
}
async function submitAndWaitErr(sk: string, addr: string, tx: any[], value = 0, gasLimit = 100000, retries = 3) {
    for (let a = 0; a < retries; a++) {
        const nonce = await getNonce(addr);
        const req = { tx, nonce: nonce.toString(), value, gasLimit, priority: 0, sender: addr, key_id: 0 };
        const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, nonce.toString(), () => {});
        if (rsp.err) { if (a+1 < retries) { await new Promise(r=>setTimeout(r,300)); continue; } throw new Error(rsp.err_str); }
        const b = await waitTx(rsp.tx_hash, 10);
        const st = b.status(rsp.tx_hash);
        const es = b.errstr(rsp.tx_hash) || "";
        if (st === "err" && es.indexOf("invalid nonce") >= 0) { if (a+1 < retries) { await new Promise(r=>setTimeout(r,300)); continue; } throw new Error(es); }
        return { rsp, block: b };
    }
    throw new Error("retries exhausted");
}

async function main() {
    await loadKeys();
    console.log("=== CREATE ADDRESS (node " + node + ") ===");

    // Проверим blake2b_hex
    const h0 = mtjs.blake2b_hex("test");
    check(typeof h0 === "string" && h0.length === 64, "blake2b_hex('test') → 64 hex");

    // bob: addr = blake2b_hex("bob"), pk = произвольный ed25519 pk (32 байта hex, 64 hex)
    const bobAddr = mtjs.blake2b_hex("bob");
    check(bobAddr.length === 64, "bob addr: 64 hex");

    // bobPk — сгенерим ed25519 пару через sign_ed? Нет. Возьмём фиктивный ed25519 pk
    // (для теста create_address реальность pk не важна — важно, что 32 байта hex)
    const bobPk = mtjs.blake2b_hex("bob_pk_fake");
    check(bobPk.length === 64, "bob pk: 64 hex (fake)");

    const info0 = await mtjs.get_address_info(node, bobAddr, 3).catch(() => ({found:false} as any));
    const bob_exists = info0.found === true;
    if (bob_exists) check(true, "bob already exists (skip t1)");

    // t1: create_address bob ok
    if (!bob_exists) {
        console.log("\n[t1] create_address(bob, [{ed25519, bobPk}]) → ok");
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "root", method: "create_address",
              params: { ttl: 10000, addr: bobAddr, keys: [{ type: 0, keybuffer: bobPk }] } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "create_address ok (errstr=" + (block.errstr(rsp.tx_hash)||"") + ")");
        const info1 = await mtjs.get_address_info(node, bobAddr, 3);
        check(info1.found === true, "bob found: true");
        check(info1.keys.length === 1, "bob keys: 1");
        if (info1.keys.length > 0) {
            check(info1.keys[0].type === 0, "bob keys[0].type == 0 (ed25519)");
            check(info1.keys[0].keybuffer === bobPk, "bob keys[0].keybuffer == bobPk");
        }
    }

    // t2: повторный create_address → err already exists
    {
        console.log("\n[t2] create_address(bob) again → err already exists");
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "root", method: "create_address",
            params: {addr: bobAddr, keys: [{ type: 0, keybuffer: bobPk }], ttl: 86400 } },
        ]);
        check(block.status(rsp.tx_hash) === "err", "status err");
        const es = block.errstr(rsp.tx_hash) || "";
        console.log("  errstr:", es);
        check(es.indexOf("already exists") >= 0, "errstr mentions already exists");
    }

    // t3: transfer на несуществующий addr → err address not found
    {
        console.log("\n[t3] transfer на несуществующий addr → err");
        const unknownAddr = mtjs.blake2b_hex("unknown_" + Date.now());
        const { rsp, block } = await submitAndWaitErr(rootSk, rootAddr, [
            { contract: "root", method: "transfer", params: { to: unknownAddr, amount: "1" } },
        ], 0);
        check(block.status(rsp.tx_hash) === "err", "status err");
        const es = block.errstr(rsp.tx_hash) || "";
        console.log("  errstr:", es);
        check(es.indexOf("address not found") >= 0, "errstr mentions address not found");
    }

    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0) console.log(GREEN + "DONE" + RESET);
    else            console.log(RED   + "FAIL" + RESET);
    std.exit(fail);
}

main().catch((e: any) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => {}, 1000000);
export {};
