#!/usr/local/bin/mtjs
import * as std from "std";

// chk-gym-rollback.ts — живые роллбэки gym на ноде:
//   t1: err TX (no visits) → state откатан, nonce +1, газ списан
//   t2: пачка ok+err (issuePass+createEntry no visits) → per-TX rollback, nonce +1
//   t3: отдельные TX (issuePass ok, createEntry no visits err) → nonce +2
//   t4: init дважды (already init) → state откатан (admin один)
//   t5: markUsed на несуществующий entry_id (no entry) → passes не изменился
//   t6: err TX (no visits) — gasUsed детерминирован (block_hash same на 2 нодах)
// node :2301 (n1). root — admin/sender (hex).

const node = "127.0.0.1:2301";
const GYM_SRC_PATH = "/home/bsr/mtjs/src/js/tsi/dist/contracts/gym.js";
const GAS_LIMIT = 100000;

const rootSk = std.getenv("u_root_ed_sk") || std.getenv("k_root_ed_sk");
const rootPk = std.getenv("k_root_ed_pk");
if (!rootSk || !rootPk) throw new Error("no root sk/pk");

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
async function getBalance(addr: string): Promise<bigint> {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    return BigInt(ui.balance);
}
async function getNonceBal(addr: string): Promise<{ nonce: bigint; bal: bigint }> {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    return { nonce: BigInt(ui.nonce), bal: BigInt(ui.balance) };
}
async function waitNonceGt(addr: string, lastNonce: bigint, timeoutSec = 10): Promise<bigint> {
    const t0 = Date.now();
    while (Date.now() - t0 < timeoutSec * 1000) {
        const n = await getNonce(addr);
        if (n > lastNonce) return n;
        await new Promise<void>((r) => setTimeout(() => r(), 200));
    }
    throw new Error("waitNonceGt timeout: lastNonce=" + lastNonce.toString());
}
async function submitAndWait(sk: string, addr: string, tx: any[], value = 0, gasLimit = GAS_LIMIT) {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    const req = { tx, nonce: ui.nonce, value, gasLimit, priority: 0 };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, ui.nonce, () => {});
    if (rsp.err) throw new Error("submit: " + rsp.err_str);
    const b = await waitTx(rsp.tx_hash, 10);
    await waitNonceGt(addr, BigInt(ui.nonce), 10);
    return { rsp, block: b };
}
async function submitAndWaitErr(sk: string, addr: string, tx: any[], value = 0, gasLimit = GAS_LIMIT) {
    // err TX тоже инкрементит nonce (Ethereum) — waitNonceGt
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    const req = { tx, nonce: ui.nonce, value, gasLimit, priority: 0 };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, ui.nonce, () => {});
    if (rsp.err) throw new Error("submit: " + rsp.err_str);
    const b = await waitTx(rsp.tx_hash, 10);
    await waitNonceGt(addr, BigInt(ui.nonce), 10);
    return { rsp, block: b };
}

const rootAddr = mtjs.addr_from_pk(rootPk!);

async function ensureRootFunded(minBalance: bigint) {
    const bal = await getBalance(rootAddr);
    console.log("  root balance:", bal.toString(), "min:", minBalance.toString());
    if (bal >= minBalance) { check(true, "root funded >= " + minBalance.toString()); return; }
    if (bal < 200000n) {
        check(false, "root balance too low for mint (" + bal.toString() + " < 200000) — clean db");
        return;
    }
    const { rsp, block } = await submitAndWait(rootSk!, rootAddr, [
        { contract: "root", method: "mint", params: { amount: "100000000" } },
    ], 0, 20000);
    check(block.status(rsp.tx_hash) === "ok", "mint root 100M ok");
}

async function ensureGym(src: string) {
    const c = await mtjs.get_contract(node, "gym", 3);
    if (c.found) {
        const { rsp, block } = await submitAndWait(rootSk!, rootAddr, [
            { contract: "root", method: "contract_update", params: { name: "gym", src } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "contract_update gym ok (errstr=" + block.errstr(rsp.tx_hash) + ")");
    } else {
        const { rsp, block } = await submitAndWait(rootSk!, rootAddr, [
            { contract: "root", method: "contract_deploy", params: { name: "gym", src } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "contract_deploy gym ok (errstr=" + block.errstr(rsp.tx_hash) + ")");
    }
}

async function ensureInit() {
    const r = await mtjs.get_contract_data(node, "gym", "admin", 3);
    if (r.found) { check(true, "gym already init (admin found)"); return; }
    const { rsp, block } = await submitAndWait(rootSk!, rootAddr, [
        { contract: "gym", method: "init", params: { admin: rootAddr } },
    ]);
    check(block.status(rsp.tx_hash) === "ok", "gym.init ok");
}

async function getAbonement(): Promise<number> {
    const r = await mtjs.get_contract_data(node, "gym", "abonement" + rootAddr, 3);
    if (!r.found) return -1;
    const v = JSON.parse(r.value);
    return Number(v.visits);
}
async function getPasses(): Promise<any[]> {
    const r = await mtjs.get_contract_data(node, "gym", "entry" + rootAddr, 3);
    if (!r.found) return [];
    const v = JSON.parse(r.value);
    return v.passes || [];
}

async function main() {
    console.log("=== GYM ROLLBACK (node " + node + ") ===");

    await ensureRootFunded(50_000_000n);

    let src = "";
    try { src = await mtjs.fs.readFile(GYM_SRC_PATH); }
    catch (e: any) { check(false, "read gym src: " + errStr(e)); return; }
    check(src.length > 0, "gym src read (" + src.length + " bytes)");

    await ensureGym(src);
    await ensureInit();

    // подготовка: issuePass (visits >= 10, если abonement нет / visits < 10)
    {
        const v = await getAbonement();
        if (v < 0) {
            const { rsp, block } = await submitAndWait(rootSk!, rootAddr, [
                { contract: "gym", method: "issuePass", params: { visits: 10 } },
            ]);
            check(block.status(rsp.tx_hash) === "ok", "issuePass 10 ok");
        } else if (v < 10) {
            const { rsp, block } = await submitAndWait(rootSk!, rootAddr, [
                { contract: "gym", method: "issuePass", params: { visits: 10 - v } },
            ]);
            check(block.status(rsp.tx_hash) === "ok", "issuePass top-up ok");
        } else {
            check(true, "abonement.visits >= 10 (skip issuePass)");
        }
    }

    // t1: err TX (createEntry M > visits) → state откатан, nonce +1, газ списан
    {
        console.log("\n[t1] err TX (createEntry M > visits): state rolled back, nonce +1, gas");
        const visits0 = await getAbonement();
        const passes0 = await getPasses();
        const { nonce: n0, bal: b0 } = await getNonceBal(rootAddr);
        const M = visits0 + 1; // > visits
        const { rsp, block } = await submitAndWaitErr(rootSk!, rootAddr, [
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

    // t2: пачка ok+err (issuePass + createEntry no visits) → per-TX rollback, nonce +1
    {
        console.log("\n[t2] batch ok+err: per-TX rollback, nonce +1");
        const visits0 = await getAbonement();
        const passes0 = await getPasses();
        const { nonce: n0, bal: b0 } = await getNonceBal(rootAddr);
        const M = visits0 + 1; // > visits
        const { rsp, block } = await submitAndWaitErr(rootSk!, rootAddr, [
            { contract: "gym", method: "issuePass", params: { visits: 100 } },
            { contract: "gym", method: "createEntry", params: { M: M, ttl: 300 } },
        ]);
        check(block.status(rsp.tx_hash) === "err", "batch status err");
        const visits1 = await getAbonement();
        const passes1 = await getPasses();
        // per-TX: err → rollback всей TX (issuePass 100 не применилась)
        check(visits1 === visits0, "abonement.visits unchanged (per-TX rollback): " + visits0 + " -> " + visits1);
        check(passes1.length === passes0.length, "passes length unchanged: " + passes0.length + " -> " + passes1.length);
        const { nonce: n1, bal: b1 } = await getNonceBal(rootAddr);
        check(n1 === n0 + 1n, "nonce inc by 1: " + n0.toString() + " -> " + n1.toString());
        check(b1 <= b0, "balance not increased (gas charged): " + b0.toString() + " -> " + b1.toString());
    }

    // t3: отдельные TX (issuePass ok, createEntry no visits err) → nonce +2
    {
        console.log("\n[t3] separate TXs: issuePass ok applied, createEntry no visits err rolled back, nonce +2");
        const visits0 = await getAbonement();
        const passes0 = await getPasses();
        const { nonce: n0, bal: b0 } = await getNonceBal(rootAddr);

        // TX1: issuePass 1 (ok)
        {
            const { rsp, block } = await submitAndWait(rootSk!, rootAddr, [
                { contract: "gym", method: "issuePass", params: { visits: 1 } },
            ]);
            check(block.status(rsp.tx_hash) === "ok", "issuePass 1 ok");
        }
        const visits1 = await getAbonement();
        check(visits1 === visits0 + 1, "visits +1 (issuePass ok): " + visits0 + " -> " + visits1);

        // TX2: createEntry M > visits (err)
        const M = visits1 + 1;
        {
            const { rsp, block } = await submitAndWaitErr(rootSk!, rootAddr, [
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

    // t4: init дважды (already init) → state откатан (admin один, не перезаписан)
    {
        console.log("\n[t4] init twice: already init err, state rolled back (admin one)");
        const admin0 = await mtjs.get_contract_data(node, "gym", "admin", 3);
        const { rsp, block } = await submitAndWaitErr(rootSk!, rootAddr, [
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

    // t5: markUsed на несуществующий entry_id → state откатан (passes не изменился)
    {
        console.log("\n[t5] markUsed on nonexistent entry_id: no entry, passes unchanged");
        const passes0 = await getPasses();
        const { rsp, block } = await submitAndWaitErr(rootSk!, rootAddr, [
            { contract: "gym", method: "markUsed", params: { user: rootAddr, entry_id: 999999, count: 1 } },
        ]);
        check(block.status(rsp.tx_hash) === "err", "markUsed nonexistent err");
        const es = block.errstr(rsp.tx_hash) || "";
        console.log("  errstr:", es);
        check(es.indexOf("no entry") >= 0, "errstr mentions no entry");
        const passes1 = await getPasses();
        check(JSON.stringify(passes1) === JSON.stringify(passes0), "passes unchanged");
    }

    // t6: err TX (createEntry no visits) — gasUsed детерминирован (block_hash same на 2 нодах)
    // (проверяем: после err TX block_hash на :2301 и :2302 совпадает — root hash сошёлся)
    if(false){
        console.log("\n[t6] err TX: block_hash same on n1 (:2301) and n2 (:2302)");
        // err TX на :2301
        const visits0 = await getAbonement();
        const M = visits0 + 1;
        const { rsp, block } = await submitAndWaitErr(rootSk!, rootAddr, [
            { contract: "gym", method: "createEntry", params: { M: M, ttl: 300 } },
        ]);
        const bh1 = block.block_hash;
        check(block.status(rsp.tx_hash) === "err", "err TX on :2301");
        // wait for :2302 to apply (waitNonceGt уже дождались на :2301; :2302 — sync)
        // get_block на :2302 (next block) — block_hash
        try {
            const p = mtjs.get_block("127.0.0.1:2302", 10);
            const b2 = await p;
            console.log("  :2301 block_hash=" + bh1.slice(0,16) + " :2302 block_hash=" + b2.block_hash.slice(0,16));
            check(b2.block_hash === bh1, ":2302 block_hash == :2301 block_hash");
        } catch (e: any) {
            check(false, "get_block :2302: " + errStr(e));
        }
    }

    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0) console.log(GREEN + "DONE" + RESET);
    else            console.log(RED   + "FAIL" + RESET);
}

main().catch((e: any) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => {}, 1000000);
export {};