#!/usr/local/bin/mtjs
import * as std from "std";
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
const rootAddr = mtjs.addr_from_name("root");
async function ensureRootFunded(minBalance) {
    const bal = await getBalance(rootAddr);
    console.log("  root balance:", bal.toString(), "min:", minBalance.toString());
    if (bal >= minBalance) {
        check(true, "root funded >= " + minBalance.toString());
        return;
    }
    if (bal < 200000n) {
        check(false, "root balance too low for mint (" + bal.toString() + " < 200000) — clean db (rm -rf db/r* db/s*) or manual mint");
        return;
    }
    const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
        { contract: "root", method: "mint", params: { amount: "100000000" } },
    ], 0, 20000);
    check(block.status(rsp.tx_hash) === "ok", "mint root 100M ok (errstr=" + block.errstr(rsp.tx_hash) + ")");
    const bal2 = await getBalance(rootAddr);
    console.log("  root balance after mint:", bal2.toString());
    check(bal2 >= minBalance, "root funded after mint >= " + minBalance.toString());
}
async function ensureGym(src) {
    const c = await mtjs.get_contract(node, "gym", 3);
    if (c.found) {
        console.log("  gym found (owner=" + c.owners.slice(0, 8) + ", src len=" + c.src.length + " → " + src.length + ")");
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
    check(block.status(rsp.tx_hash) === "ok", "gym.init ok (errstr=" + block.errstr(rsp.tx_hash) + ")");
}
function findActiveId(passes, now) {
    for (const pp of passes) {
        if (!pp.disabled && Number(pp.used) < Number(pp.M) && Number(pp.expires_at) > now)
            return Number(pp.id);
    }
    return -1;
}
async function main() {
    await loadKeys();
    console.log("=== GYM LIVE (node " + node + ") ===");
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
        const r = await mtjs.get_contract_data(node, "gym", "abonement" + rootAddr, 3);
        if (r.found) {
            check(true, "abonement already exists (skip issuePass)");
        }
        else {
            const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
                { contract: "gym", method: "issuePass", params: { visits: 10 } },
            ]);
            check(block.status(rsp.tx_hash) === "ok", "gym.issuePass ok (errstr=" + block.errstr(rsp.tx_hash) + ")");
        }
    }
    {
        const r = await mtjs.get_contract_data(node, "gym", "abonement" + rootAddr, 3);
        console.log("  abonement:", JSON.stringify(r));
        check(r.found === true, "abonement found");
        if (r.found) {
            let v = null;
            try {
                v = JSON.parse(r.value);
            }
            catch (e) {
                check(false, "abonement value JSON: " + String(e));
            }
            if (v)
                check(Number(v.visits) >= 0, "abonement.visits >= 0 (got " + (v && v.visits) + ")");
        }
    }
    {
        const now = Math.floor(Date.now() / 1000);
        const r = await mtjs.get_contract_data(node, "gym", "entry" + rootAddr, 3);
        let activeId = -1;
        if (r.found) {
            try {
                const v = JSON.parse(r.value);
                activeId = findActiveId(v.passes || [], now);
            }
            catch (e) { }
        }
        if (activeId >= 0) {
            check(true, "active entry exists (id=" + activeId + ", skip createEntry)");
        }
        else {
            const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
                { contract: "gym", method: "createEntry", params: { M: 3, ttl: 300 } },
            ]);
            check(block.status(rsp.tx_hash) === "ok", "gym.createEntry ok (errstr=" + block.errstr(rsp.tx_hash) + ")");
        }
    }
    {
        const r = await mtjs.get_contract_data(node, "gym", "entry" + rootAddr, 3);
        console.log("  entry:", JSON.stringify(r));
        check(r.found === true, "entry found");
    }
    {
        const now = Math.floor(Date.now() / 1000);
        const r = await mtjs.get_contract_data(node, "gym", "entry" + rootAddr, 3);
        let activeId = -1;
        if (r.found) {
            try {
                const v = JSON.parse(r.value);
                activeId = findActiveId(v.passes || [], now);
            }
            catch (e) { }
        }
        if (activeId < 0) {
            check(false, "no active entry for markUsed");
        }
        else {
            const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
                { contract: "gym", method: "markUsed", params: { user: rootAddr, entry_id: activeId, count: 1 } },
            ]);
            check(block.status(rsp.tx_hash) === "ok", "gym.markUsed ok (id=" + activeId + ", errstr=" + block.errstr(rsp.tx_hash) + ")");
        }
    }
    {
        const r = await mtjs.get_contract_data(node, "gym", "entry" + rootAddr, 3);
        if (r.found) {
            try {
                const v = JSON.parse(r.value);
                const passes = v.passes || [];
                const usedAny = passes.some((pp) => Number(pp.used) >= 1);
                check(usedAny, "some entry.passes[].used >= 1");
            }
            catch (e) {
                check(false, "entry JSON: " + String(e));
            }
        }
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
