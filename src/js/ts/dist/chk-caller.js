#!/usr/local/bin/mtjs
import * as std from "std";
const node = "127.0.0.1:2301";
const GAS_LIMIT = 100000;
const CALLER_A_SRC_PATH = "/home/bsr/mtjs/src/js/tsi/dist/contracts/caller_a.js";
const CALLER_B_SRC_PATH = "/home/bsr/mtjs/src/js/tsi/dist/contracts/caller_b.js";
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
async function getBalance(addr) {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    return BigInt(ui.balance);
}
async function submitAndWait(sk, addr, tx, value = 0, gasLimit = GAS_LIMIT, retries = 5) {
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
async function submitAndWaitErr(sk, addr, tx, value = 0, gasLimit = GAS_LIMIT, retries = 3) {
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
    throw new Error("submitAndWaitErr: retries exhausted");
}
const rootAddr = mtjs.addr_from_name("root");
async function ensureRootFunded(minBalance) {
    const bal = await getBalance(rootAddr);
    console.log("  root balance:", bal.toString(), "min:", minBalance.toString());
    if (bal >= minBalance) {
        check(true, "root funded >= " + minBalance.toString());
        return;
    }
    if (bal < 200000n) {
        check(false, "root balance too low for mint");
        return;
    }
    const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
        { contract: "root", method: "mint", params: { amount: "100000000" } },
    ], 0, 20000);
    check(block.status(rsp.tx_hash) === "ok", "mint root 100M ok");
}
async function ensureContract(name, srcPath) {
    let src = "";
    try {
        src = await mtjs.fs.readFile(srcPath);
    }
    catch (e) {
        check(false, "read " + name + " src: " + errStr(e));
        return;
    }
    check(src.length > 0, name + " src read (" + src.length + " bytes)");
    const c = await mtjs.get_contract(node, name, 3);
    if (c.found) {
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "root", method: "contract_update", params: { name: name, src: src } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "contract_update " + name + " ok (errstr=" + block.errstr(rsp.tx_hash) + ")");
    }
    else {
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "root", method: "contract_deploy", params: { name: name, src: src } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "contract_deploy " + name + " ok (errstr=" + block.errstr(rsp.tx_hash) + ")");
    }
}
function childEmits(block, txHash, seqId) {
    const ch = block.children(txHash);
    if (!ch)
        return [];
    const nd = ch[String(seqId)];
    if (!nd || !nd.emits)
        return [];
    const out = [];
    for (const e of nd.emits) {
        let val = e[1];
        if (typeof val === "string") {
            try {
                val = JSON.parse(val);
            }
            catch (_) { }
        }
        out.push({ name: e[0], value: val });
    }
    return out;
}
function findEmit(list, name) {
    for (const e of list)
        if (e.name === name)
            return e.value;
    return null;
}
async function main() {
    await loadKeys();
    console.log("=== CALLER TESTS (node " + node + ") ===");
    await ensureRootFunded(50000000n);
    await ensureContract("caller_a", CALLER_A_SRC_PATH);
    await ensureContract("caller_b", CALLER_B_SRC_PATH);
    {
        console.log("\n[1] external caller_a.whoAmI -> caller=\"\", sender=root");
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "caller_a", method: "whoAmI", params: {} },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "status ok (errstr=" + block.errstr(rsp.tx_hash) + ")");
        const em = childEmits(block, rsp.tx_hash, 0);
        console.log("  emits[0]:", JSON.stringify(em));
        const r = findEmit(em, "whoAmI");
        check(r !== null, "whoAmI emit present");
        if (r) {
            check(r.caller === "", "caller == \"\" (got '" + r.caller + "')");
            check(r.sender === rootAddr, "sender == root (got " + r.sender + ")");
        }
    }
    {
        console.log("\n[2] A.callB -> B.whoCalled -> caller=\"caller_a\", sender=root");
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "caller_a", method: "callB", params: {} },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "status ok (errstr=" + block.errstr(rsp.tx_hash) + ")");
        const em = childEmits(block, rsp.tx_hash, 0);
        console.log("  emits[0]:", JSON.stringify(em));
        const r = findEmit(em, "whoCalled");
        check(r !== null, "whoCalled emit present");
        if (r) {
            check(r.caller === "caller_a", "caller == \"caller_a\" (got '" + r.caller + "')");
            check(r.sender === rootAddr, "sender == root (got " + r.sender + ")");
        }
    }
    {
        console.log("\n[3] external B.onlyA -> err (\"only caller_a\")");
        const { rsp, block } = await submitAndWaitErr(rootSk, rootAddr, [
            { contract: "caller_b", method: "onlyA", params: {} },
        ]);
        check(block.status(rsp.tx_hash) === "err", "status err");
        const es = block.errstr(rsp.tx_hash) || "";
        console.log("  errstr:", es);
        check(es.indexOf("only caller_a") >= 0, "errstr mentions only caller_a");
    }
    {
        console.log("\n[4] external B.whoCalled -> caller=\"\"");
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "caller_b", method: "whoCalled", params: {} },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "status ok (errstr=" + block.errstr(rsp.tx_hash) + ")");
        const em = childEmits(block, rsp.tx_hash, 0);
        console.log("  emits[0]:", JSON.stringify(em));
        const r = findEmit(em, "whoCalled");
        check(r !== null, "whoCalled emit present");
        if (r)
            check(r.caller === "", "caller == \"\" (got '" + r.caller + "')");
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
