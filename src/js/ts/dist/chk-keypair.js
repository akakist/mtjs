#!/usr/local/bin/mtjs
import * as std from "std";
// chk-keypair.ts — generate_keypair для 4 типов (ed25519/secp256k1/p256/bls12-381)
//                 + create_address + transfer + node_create (BLS).
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
async function main() {
    await loadKeys();
    console.log("=== KEYPAIR (node " + node + ") ===");
    await ensureRootFunded(50000000n);
    // ============================================================
    // [t1] generate_keypair для 4 типов (10 ключей каждый)
    // ============================================================
    const types = [
        { t: 0, name: "ed25519", sk_len: 128, pk_len: 64 },
        { t: 1, name: "secp256k1", sk_len: 64, pk_len: 66 },
        { t: 2, name: "p256", sk_len: 64, pk_len: 66 },
        { t: 3, name: "bls12-381", sk_len: 64, pk_len: 96 },
    ];
    for (const ty of types) {
        console.log("\n[t1." + ty.t + "] generate_keypair(" + ty.name + ") × 10");
        let ok = 0;
        for (let i = 0; i < 10; i++) {
            const kp = mtjs.generate_keypair(ty.t);
            if (kp.type !== ty.t) {
                check(false, ty.name + " type mismatch");
                continue;
            }
            if (typeof kp.sk !== "string" || kp.sk.length !== ty.sk_len) {
                check(false, ty.name + " sk len " + (kp.sk ? kp.sk.length : "undefined") + " != " + ty.sk_len);
                continue;
            }
            if (typeof kp.pk !== "string" || kp.pk.length !== ty.pk_len) {
                check(false, ty.name + " pk len " + (kp.pk ? kp.pk.length : "undefined") + " != " + ty.pk_len);
                continue;
            }
            ok++;
        }
        check(ok === 10, ty.name + " 10/10 keys valid");
    }
    // ============================================================
    // [t2] create_address с ed25519 ключом + проверка
    // ============================================================
    console.log("\n[t2] create_address с generate_keypair(0)");
    const kp0 = mtjs.generate_keypair(0);
    const addr0 = mtjs.blake2b_hex("gc_keypair_" + Date.now());
    {
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "root", method: "create_address", params: {
                    addr: addr0,
                    keys: [{ type: 0, keybuffer: kp0.pk }],
                    ttl: 100000,
                } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "create_address ok (errstr=" + (block.errstr(rsp.tx_hash) || "") + ")");
    }
    {
        const info = await mtjs.get_address_info(node, addr0, 3);
        check(info.found === true, "address found");
        check(info.keys.length === 1, "1 key");
        if (info.keys.length > 0) {
            check(info.keys[0].type === 0, "key type ed25519");
            check(info.keys[0].keybuffer === kp0.pk, "keybuffer matches");
        }
    }
    // ============================================================
    // [t3] transfer от сгенерированного адреса (подпись sign_ed)
    //     addr0 баланс 1000 → пополняем до 50000, gasLimit=20000, value=10
    // ============================================================
    console.log("\n[t3] transfer от сгенерированного адреса");
    {
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "root", method: "transfer", params: { to: addr0, amount: "50000" } },
        ], 50000, 20000);
        check(block.status(rsp.tx_hash) === "ok", "transfer to addr0 ok");
    }
    {
        const before = await getNonceBal(rootAddr);
        const { rsp, block } = await submitAndWait(kp0.sk, addr0, [
            { contract: "root", method: "transfer", params: { to: rootAddr, amount: "10" } },
        ], 10, 20000); // ← gasLimit=20000, value=10
        check(block.status(rsp.tx_hash) === "ok", "transfer from addr0 ok (errstr=" + (block.errstr(rsp.tx_hash) || "") + ")");
        const after = await getNonceBal(rootAddr);
        check(after.bal > before.bal, "root received 10 (bal " + before.bal.toString() + " -> " + after.bal.toString() + ")");
    }
    // ============================================================
    // [t4] create_address с secp256k1 ключом
    // ============================================================
    console.log("\n[t4] create_address с generate_keypair(1) — secp256k1");
    const kp1 = mtjs.generate_keypair(1);
    const addr1 = mtjs.blake2b_hex("gc_keypair_secp_" + Date.now());
    {
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "root", method: "create_address", params: {
                    addr: addr1,
                    keys: [{ type: 1, keybuffer: kp1.pk }],
                    ttl: 100000,
                } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "create_address secp256k1 ok (errstr=" + (block.errstr(rsp.tx_hash) || "") + ")");
    }
    {
        const info = await mtjs.get_address_info(node, addr1, 3);
        check(info.found === true, "secp256k1 address found");
        if (info.keys.length > 0) {
            check(info.keys[0].type === 1, "key type secp256k1");
            check(info.keys[0].keybuffer === kp1.pk, "keybuffer matches");
        }
    }
    // ============================================================
    // [t5] create_address с p256 ключом
    // ============================================================
    console.log("\n[t5] create_address с generate_keypair(2) — p256");
    const kp2 = mtjs.generate_keypair(2);
    const addr2 = mtjs.blake2b_hex("gc_keypair_p256_" + Date.now());
    {
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "root", method: "create_address", params: {
                    addr: addr2,
                    keys: [{ type: 2, keybuffer: kp2.pk }],
                    ttl: 100000,
                } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "create_address p256 ok (errstr=" + (block.errstr(rsp.tx_hash) || "") + ")");
    }
    {
        const info = await mtjs.get_address_info(node, addr2, 3);
        check(info.found === true, "p256 address found");
        if (info.keys.length > 0) {
            check(info.keys[0].type === 2, "key type p256");
            check(info.keys[0].keybuffer === kp2.pk, "keybuffer matches");
        }
    }
    // ============================================================
    // [t6] node_create с BLS (generate_keypair(3)) + ed25519 (generate_keypair(0))
    // ============================================================
    console.log("\n[t6] node_create с generate_keypair(3).pk (BLS)");
    const kpB = mtjs.generate_keypair(3); // BLS
    const kpE = mtjs.generate_keypair(0); // ed25519
    const nodeName = "n_keypair_test_" + Date.now();
    {
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "root", method: "node_create", params: {
                    name: nodeName,
                    ip: "127.0.0.1:9999",
                    pk_ed: kpE.pk,
                    pk_bls: kpB.pk,
                    ttl: 100000,
                } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "node_create with BLS ok (errstr=" + (block.errstr(rsp.tx_hash) || "") + ")");
    }
    {
        const ni = await mtjs.get_node_info(node, nodeName, 3);
        check(ni.found === true, "node found");
        check(ni.bls_pk === kpB.pk, "bls_pk matches");
        check(ni.ed_pk === kpE.pk, "ed_pk matches");
    }
    // ============================================================
    // [t7] BLS sk/pk consistency (длины hex)
    // ============================================================
    console.log("\n[t7] BLS sk/pk consistency");
    check(kpB.sk.length === 64, "BLS sk 32 bytes (64 hex)");
    check(kpB.pk.length === 96, "BLS pk 48 bytes (96 hex)");
    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0)
        console.log(GREEN + "DONE" + RESET);
    else
        console.log(RED + "FAIL" + RESET);
    std.exit(fail);
}
main().catch((e) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => { }, 1000000);
