#!/usr/local/bin/mtjs
import * as std from "std";
// chk-gym-rollback.ts — живые роллбэки gym на ноде:
//   t1: err TX (no visits) → state откатан, nonce +1, газ списан
//   t2: пачка ok+err (issuePass+createEntry no visits) → per-TX rollback, nonce +1
//   t3: отдельные TX (issuePass ok, createEntry no visits err) → nonce +2
//   t4: init дважды (already init) → state откатан (admin один)
//   t5: markUsed на несуществующий entry_id (no entry) → passes не изменился
// node :2301 (n1). root — admin/sender (hex).
const node = "127.0.0.1:2301";
const GYM_SRC_PATH = "/home/bsr/mtjs/src/js/tsi/dist/contracts/gym.js";
const GAS_LIMIT = 100000;
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
async function getNonceBal(addr) {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    return { nonce: BigInt(ui.nonce), bal: BigInt(ui.balance) };
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
async function submitAndWaitErr(sk, addr, tx, value = 0, gasLimit = GAS_LIMIT, retries = 5) {
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
        let b;
        try {
            b = await waitTx(rsp.tx_hash, 30);
        }
        catch (e) {
            if (attempt + 1 < retries) {
                await new Promise(r => setTimeout(r, 300));
                continue;
            }
            throw e;
        }
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
        check(false, "root balance too low for mint (" + bal.toString() + " < 200000) — clean db");
        return;
    }
    const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
        { contract: "root", method: "mint", params: { amount: "100000000" } },
    ], 0, 20000);
    check(block.status(rsp.tx_hash) === "ok", "mint root 100M ok");
}
async function ensureGym(src) {
    const c = await mtjs.get_contract(node, "gym", 3);
    if (c.found) {
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "root", method: "contract_update", params: { name: "gym", src } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "contract_update gym ok (errstr=" + block.errstr(rsp.tx_hash) + ")");
    }
    else {
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "root", method: "contract_deploy", params: { name: "gym", src } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "contract_deploy gym ok (errstr=" + block.errstr(rsp.tx_hash) + ")");
    }
}
async function ensureInit() {
    const r = await mtjs.get_contract_data(node, "gym", "admin", 3);
    if (r.found) {
        check(true, "gym already init (admin found)");
        return;
    }
    const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
        { contract: "gym", method: "init", params: { admin: rootAddr } },
    ]);
    check(block.status(rsp.tx_hash) === "ok", "gym.init ok");
}
async function getAbonement() {
    const r = await mtjs.get_contract_data(node, "gym", "abonement" + rootAddr, 3);
    if (!r.found)
        return -1;
    const v = JSON.parse(r.value);
    return Number(v.visits);
}
async function getPasses() {
    const r = await mtjs.get_contract_data(node, "gym", "entry" + rootAddr, 3);
    if (!r.found)
        return [];
    const v = JSON.parse(r.value);
    return v.passes || [];
}
async function main() {
    await loadKeys();
    console.log("=== GYM ROLLBACK (node " + node + ") ===");
    await ensureRootFunded(50000000n);
    let src = "";
    try {
        src = await mtjs.fs.readFile(GYM_SRC_PATH);
    }
    catch (e) {
        check(false, "read gym src: " + errStr(e));
        return;
    }
    check(src.length > 0, "gym src read (" + src.length + " bytes)");
    await ensureGym(src);
    await ensureInit();
    {
        const v = await getAbonement();
        if (v < 0) {
            const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
                { contract: "gym", method: "issuePass", params: { visits: 10 } },
            ]);
            check(block.status(rsp.tx_hash) === "ok", "issuePass 10 ok");
        }
        else if (v < 10) {
            const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
                { contract: "gym", method: "issuePass", params: { visits: 10 - v } },
            ]);
            check(block.status(rsp.tx_hash) === "ok", "issuePass top-up ok");
        }
        else {
            check(true, "abonement.visits >= 10 (skip issuePass)");
        }
    }
    {
        console.log("\n[t1] err TX (createEntry M > visits): state rolled back, nonce +1, gas");
        const visits0 = await getAbonement();
        const passes0 = await getPasses();
        const { nonce: n0, bal: b0 } = await getNonceBal(rootAddr);
        const M = visits0 + 1;
        const { rsp, block } = await submitAndWaitErr(rootSk, rootAddr, [
            { contract: "gym", method: "createEntry", params: { M: M, ttl: 300 } },
        ]);
        check(block.status(rsp.tx_hash) === "err", "status err");
        const es = block.errstr(rsp.tx_hash) || "";
        console.log("  errstr:", es);
        check(es.indexOf("no visits") >= 0, "errstr mentions no visits");
        const visits1 = await getAbonement();
        const passes1 = await getPasses();
        check(visits1 === visits0, "abonement.visits unchanged: " + visits0 + " -> " + visits1);
        check(passes1.length === passes0.length, "passes length unchanged: " + passes0.length + " -> " + passes1.length);
        const { nonce: n1, bal: b1 } = await getNonceBal(rootAddr);
        check(n1 === n0 + 1n, "nonce inc by 1: " + n0.toString() + " -> " + n1.toString());
        check(b1 <= b0, "balance not increased (gas charged): " + b0.toString() + " -> " + b1.toString());
    }
    {
        console.log("\n[t2] batch ok+err: per-TX rollback, nonce +1");
        const visits0 = await getAbonement();
        const passes0 = await getPasses();
        const { nonce: n0, bal: b0 } = await getNonceBal(rootAddr);
        const M = visits0 + 1;
        const { rsp, block } = await submitAndWaitErr(rootSk, rootAddr, [
            { contract: "gym", method: "issuePass", params: { visits: 100 } },
            { contract: "gym", method: "createEntry", params: { M: M, ttl: 300 } },
        ]);
        check(block.status(rsp.tx_hash) === "err", "batch status err");
        const visits1 = await getAbonement();
        const passes1 = await getPasses();
        check(visits1 === visits0, "abonement.visits unchanged (per-TX rollback): " + visits0 + " -> " + visits1);
        check(passes1.length === passes0.length, "passes length unchanged: " + passes0.length + " -> " + passes1.length);
        const { nonce: n1, bal: b1 } = await getNonceBal(rootAddr);
        check(n1 === n0 + 1n, "nonce inc by 1: " + n0.toString() + " -> " + n1.toString());
        check(b1 <= b0, "balance not increased (gas charged): " + b0.toString() + " -> " + b1.toString());
    }
    {
        console.log("\n[t3] separate TXs: issuePass ok applied, createEntry no visits err rolled back, nonce +2");
        const visits0 = await getAbonement();
        const passes0 = await getPasses();
        const { nonce: n0, bal: b0 } = await getNonceBal(rootAddr);
        {
            const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
                { contract: "gym", method: "issuePass", params: { visits: 1 } },
            ]);
            check(block.status(rsp.tx_hash) === "ok", "issuePass 1 ok");
        }
        const visits1 = await getAbonement();
        check(visits1 === visits0 + 1, "visits +1 (issuePass ok): " + visits0 + " -> " + visits1);
        const M = visits1 + 1;
        {
            const { rsp, block } = await submitAndWaitErr(rootSk, rootAddr, [
                { contract: "gym", method: "createEntry", params: { M: M, ttl: 300 } },
            ]);
            check(block.status(rsp.tx_hash) === "err", "createEntry M>visits err");
        }
        const visits2 = await getAbonement();
        const passes2 = await getPasses();
        check(visits2 === visits1, "visits unchanged after err: " + visits1 + " -> " + visits2);
        check(passes2.length === passes0.length, "passes length unchanged after err: " + passes0.length + " -> " + passes2.length);
        const { nonce: n2, bal: b2 } = await getNonceBal(rootAddr);
        check(n2 === n0 + 2n, "nonce +2: " + n0.toString() + " -> " + n2.toString());
        check(b2 <= b0, "balance not increased (gas): " + b0.toString() + " -> " + b2.toString());
    }
    {
        console.log("\n[t4] init twice: already init err, state rolled back (admin one)");
        const admin0 = await mtjs.get_contract_data(node, "gym", "admin", 3);
        const { rsp, block } = await submitAndWaitErr(rootSk, rootAddr, [
            { contract: "gym", method: "init", params: { admin: "deadbeef" } },
        ]);
        check(block.status(rsp.tx_hash) === "err", "init twice err");
        const es = block.errstr(rsp.tx_hash) || "";
        console.log("  errstr:", es);
        check(es.indexOf("already init") >= 0, "errstr mentions already init");
        const admin1 = await mtjs.get_contract_data(node, "gym", "admin", 3);
        check(admin1.found === true, "admin still found");
        check(admin1.value === admin0.value, "admin unchanged: " + admin0.value + " -> " + admin1.value);
    }
    {
        console.log("\n[t5] markUsed on nonexistent entry_id: no entry, passes unchanged");
        const passes0 = await getPasses();
        const { rsp, block } = await submitAndWaitErr(rootSk, rootAddr, [
            { contract: "gym", method: "markUsed", params: { user: rootAddr, entry_id: 999999, count: 1 } },
        ]);
        check(block.status(rsp.tx_hash) === "err", "markUsed nonexistent err");
        const es = block.errstr(rsp.tx_hash) || "";
        console.log("  errstr:", es);
        check(es.indexOf("no entry") >= 0, "errstr mentions no entry");
        const passes1 = await getPasses();
        check(JSON.stringify(passes1) === JSON.stringify(passes0), "passes unchanged");
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
