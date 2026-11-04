#!/usr/local/bin/mtjs
import * as std from "std";
const node = "127.0.0.1:2301";
const KEYS_PATH = std.getenv("MTJS_KEYS_PATH") || "keys_test.json";
let rootSk = "";
let rootPk = "";
async function loadKeys() {
    const raw = await mtjs.fs.readFile(KEYS_PATH);
    const keys = JSON.parse(raw);
    const r = keys.users?.root?.keys?.[0];
    if (!r?.sk || !r?.pk)
        throw new Error("keys_test.json: no users.root.keys[0].sk/pk");
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
    if (e && typeof e === "object" && ("errcode" in e || "errstr" in e))
        return "errcode=" + e.errcode + " errstr=" + e.errstr;
    return String(e);
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
        const timer = setTimeout(() => {
            delete pending[h];
            rej({ errcode: 1, errstr: "timed out (no block with tx " + h.slice(0, 8) + ")" });
        }, to * 1000);
        pending[h] = { resolve: res, reject: rej, timer };
    });
}
async function getNonce(addr) {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    console.log("getNonce: " + ui.nonce);
    return BigInt(ui.nonce);
}
async function getNonceBal(addr) {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    return { nonce: BigInt(ui.nonce), bal: BigInt(ui.balance) };
}
async function submitAndWait(sk, addr, tx, value = 0, gasLimit = 20000, retries = 5) {
    for (let attempt = 0; attempt < retries; attempt++) {
        const nonce = await getNonce(addr);
        const req = { tx, nonce: nonce.toString(), value, gasLimit, priority: 0, sender: addr, key_id: 0 };
        const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, nonce.toString(), () => { });
        if (rsp.err) {
            if (attempt + 1 < retries) {
                await new Promise(r => setTimeout(r, 300));
                continue;
            }
            throw new Error("submit: " + rsp.err_str);
        }
        const b = await waitTx(rsp.tx_hash, 10);
        const st = b.status(rsp.tx_hash);
        const es = b.errstr(rsp.tx_hash) || "";
        if (st === "err" && es.indexOf("invalid nonce") >= 0) {
            if (attempt + 1 < retries) {
                await new Promise(r => setTimeout(r, 300));
                continue;
            }
            throw new Error("invalid nonce: " + es);
        }
        return { rsp, block: b, nonce };
    }
    throw new Error("submitAndWait: retries exhausted");
}
async function mintFromRoot(amount) {
    const rootAddr = mtjs.addr_from_name("root");
    return await submitAndWait(rootSk, rootAddr, [
        { contract: "root", method: "mint", params: { amount } },
    ], 0, 20000);
}
async function transferFromRoot(to, amount) {
    const rootAddr = mtjs.addr_from_name("root");
    return await submitAndWait(rootSk, rootAddr, [
        { contract: "root", method: "transfer", params: { to, amount } },
    ], 100000, 20000);
}
function childEmits(b, txHash) {
    const ch = b.children(txHash);
    const out = {};
    if (ch)
        for (const k of Object.keys(ch))
            out[k] = ch[k].emits;
    return out;
}
function findChildEmit(b, txHash, name) {
    const em = childEmits(b, txHash);
    for (const k of Object.keys(em))
        for (const e of em[k])
            if (e[0] === name)
                return e[1];
    return null;
}
async function t1_mint_root() {
    console.log("\n[t1] mint root -> root");
    const rootAddr = mtjs.addr_from_name("root");
    const { bal: b0 } = await getNonceBal(rootAddr);
    console.log("  root balance before:", b0.toString());
    const amount = "5000000";
    const { rsp, block } = await mintFromRoot(amount);
    const st = block.status(rsp.tx_hash);
    const es = block.errstr(rsp.tx_hash);
    console.log("  status:", st, "errstr:", es);
    check(st === "ok", "status ok");
    const mintEmit = findChildEmit(block, rsp.tx_hash, "mint");
    check(mintEmit !== null, "mint emit present (in children)");
    if (mintEmit) {
        check(mintEmit.to === rootAddr, "mint to == root");
        check(mintEmit.amount === amount, "mint amount == " + amount);
    }
    const { bal: b1 } = await getNonceBal(rootAddr);
    console.log("  root balance after:", b1.toString());
    check(b1 > b0, "balance grew: " + b0.toString() + " -> " + b1.toString());
    check(b1 >= b0 + 5000000n - 10000n, "balance grew by ~amount (>= amount - 10000)");
}
async function t2_mint_all_users() {
    console.log("\n[t2] mint root + transfer u0..u19");
    const rootAddr = mtjs.addr_from_name("root");
    const amount = "100000";
    const { rsp, block } = await mintFromRoot(amount);
    const st = block.status(rsp.tx_hash);
    check(st === "ok", "mint root " + amount + " ok");
    if (st !== "ok") {
        console.log("  errstr:", block.errstr(rsp.tx_hash));
        return;
    }
    for (let i = 0; i < 20; i++) {
        const uSk = std.getenv("k_u" + i + "_ed_sk");
        const uPk = std.getenv("k_u" + i + "_ed_pk");
        if (!uSk || !uPk) {
            console.log("  u" + i + ": no keys, skip");
            continue;
        }
        const uAddr = mtjs.addr_from_name("u" + i);
        const { bal: ub0 } = await getNonceBal(uAddr);
        try {
            const { rsp: trsp, block: tb } = await transferFromRoot(uAddr, "1000");
            const tst = tb.status(trsp.tx_hash);
            if (tst !== "ok") {
                check(false, "u" + i + " transfer status=" + tst + " errstr=" + tb.errstr(trsp.tx_hash));
                continue;
            }
            const trEmit = findChildEmit(tb, trsp.tx_hash, "transfer");
            check(trEmit !== null, "u" + i + " transfer emit present");
            const { bal: ub1 } = await getNonceBal(uAddr);
            check(ub1 === ub0 + 1000n, "u" + i + " +1000: " + ub0.toString() + " -> " + ub1.toString());
        }
        catch (e) {
            check(false, "u" + i + " transfer: " + errStr(e));
        }
    }
}
async function t3_mint_zero() {
    console.log("\n[t3] mint 0 (граница)");
    const rootAddr = mtjs.addr_from_name("root");
    const { bal: b0 } = await getNonceBal(rootAddr);
    const { rsp, block } = await mintFromRoot("0");
    const st = block.status(rsp.tx_hash);
    console.log("  status:", st, "errstr:", block.errstr(rsp.tx_hash));
    check(st === "ok", "status ok");
    const { bal: b1 } = await getNonceBal(rootAddr);
    check(b1 <= b0, "balance not increased (gas only): " + b0.toString() + " -> " + b1.toString());
}
(async () => {
    await loadKeys();
    console.log("=== MINT ALL TESTS ===");
    await t1_mint_root();
    await t2_mint_all_users();
    await t3_mint_zero();
    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0)
        console.log(GREEN + "DONE" + RESET);
    else
        console.log(RED + "FAIL" + RESET);
    std.exit(fail);
})().catch((e) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => { }, 1000000);
