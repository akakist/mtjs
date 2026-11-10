#!/usr/local/bin/mtjs
import * as std from "std";
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
        { contract: "root", method: "mint", params: { amount: "1000000000" } },
    ], 0, 20000);
}
async function ensureTickets(src) {
    const c = await mtjs.get_contract(node, "tickets", 3);
    const method = c.found ? "contract_update" : "contract_deploy";
    await submitAndWait(rootSk, rootAddr, [
        { contract: "root", method, params: { name: "tickets", src, ttl: 1000000 } },
    ]);
}
async function main() {
    await loadKeys();
    console.log("=== TICKETS (node " + node + ") ===");
    await ensureRootFunded(1000000000n);
    const TICKETS_SRC = "/home/bsr/mtjs/src/js/tsi/dist/contracts/tickets.js";
    const src = await mtjs.fs.readFile(TICKETS_SRC);
    check(src.length > 0, "tickets src read (" + src.length + " bytes)");
    await ensureTickets(src);
    // уникальное имя event (каждый запуск — свой)
    const EVENT_NAME = "AC/DC_" + Date.now();
    console.log("  event name:", EVENT_NAME);
    // ============================================================
    // [t1] createEvent: 10 sectors × 10 rows × 10 seats = 1000 мест
    // ============================================================
    console.log("\n[t1] createEvent 10×10×10 = 1000 мест");
    {
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "tickets", method: "createEvent", params: {
                    name: EVENT_NAME, sectors: 10, rows: 10, seats: 10, price: 10,
                } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "createEvent ok (errstr=" + (block.errstr(rsp.tx_hash) || "") + ")");
    }
    // ============================================================
    // [t2] getEvent
    // ============================================================
    console.log("\n[t2] getEvent");
    {
        const r = await mtjs.call_view(node, "tickets", "getEvent", { event: EVENT_NAME }, 10, 10);
        check(r.found === true && !r.error, "view getEvent ok");
        if (r.found && !r.error) {
            const v = JSON.parse(r.result);
            check(v.found === true, "event found");
            check(v.total === 1000, "total=1000 (got " + v.total + ")");
            check(v.sold === 0, "sold=0 (got " + v.sold + ")");
        }
    }
    // ============================================================
    // [t3] buyTicket × 100 (10 юзеров × 10) — gasLimit=20000, баланс 100000
    // ============================================================
    console.log("\n[t3] buyTicket × 100 (10 юзеров × 10)");
    const USERS = 10;
    const TICKETS_PER_USER = 10;
    const uKeys = JSON.parse(await mtjs.fs.readFile(KEYS_PATH));
    for (let ui = 0; ui < USERS; ui++) {
        const uName = "u" + ui;
        const uAddr = mtjs.addr_from_name(uName);
        const uSk = uKeys.users?.[uName]?.keys?.[0]?.sk;
        if (!uSk) {
            console.log("  skip " + uName + " (no sk)");
            continue;
        }
        // пополнить (100000 — с запасом на gasLimit=20000 + value)
        const ui0 = await mtjs.get_user_nonce(node, uAddr, 3);
        if (BigInt(ui0.balance) < 100000n) {
            await submitAndWait(rootSk, rootAddr, [
                { contract: "root", method: "transfer", params: { to: uAddr, amount: "100000" } },
            ], 100000, 20000);
        }
        for (let ti = 0; ti < TICKETS_PER_USER; ti++) {
            const sector = ui;
            const row = Math.floor(ti / 10);
            const seat = ti % 10;
            try {
                const { rsp, block } = await submitAndWait(uSk, uAddr, [
                    { contract: "tickets", method: "buyTicket", params: {
                            event: EVENT_NAME, sector, row, seat, amount: 10,
                        } },
                ], 10, 20000); // ← gasLimit=20000
                if (block.status(rsp.tx_hash) !== "ok") {
                    check(false, uName + " buyTicket [" + sector + "," + row + "," + seat + "]: " + (block.errstr(rsp.tx_hash) || ""));
                }
            }
            catch (e) {
                check(false, uName + " buyTicket [" + sector + "," + row + "," + seat + "]: " + errStr(e));
            }
        }
    }
    // ============================================================
    // [t4] getEvent — sold=100
    // ============================================================
    console.log("\n[t4] getEvent — sold=100");
    {
        const r = await mtjs.call_view(node, "tickets", "getEvent", { event: EVENT_NAME }, 10, 10);
        if (r.found && !r.error) {
            const v = JSON.parse(r.result);
            check(v.sold === 100, "sold=100 (got " + v.sold + ")");
        }
    }
    // ============================================================
    // [t5] getTicket для купленного
    // ============================================================
    console.log("\n[t5] getTicket");
    {
        const r = await mtjs.call_view(node, "tickets", "getTicket", { event: EVENT_NAME, sector: 0, row: 0, seat: 0 }, 10, 10);
        if (r.found && !r.error) {
            const v = JSON.parse(r.result);
            check(v.found === true, "ticket found");
            check(v.sold === true, "sold=true");
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
