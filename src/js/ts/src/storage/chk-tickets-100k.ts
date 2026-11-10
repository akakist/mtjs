#!/usr/local/bin/mtjs
import * as std from "std";

const KEYS_PATH = std.getenv("MTJS_KEYS_PATH") || "keys_test.json";
const GAS_LIMIT = 20000;
const NODES = 10;
const USERS = 100;
const TICKETS_PER_USER = 1000;   // 100 × 1000 = 100k
const BATCH = 100;
const USER_CONCURRENCY = 10;

function nodeAddr(i: number): string {
    return "127.0.0.1:" + (2300 + (i % NODES));
}

let rootSk = ""; let rootPk = "";
async function loadKeys() {
    const raw = await mtjs.fs.readFile(KEYS_PATH);
    const keys = JSON.parse(raw);
    const r = keys.users?.root?.keys?.[0];
    if (!r?.sk || !r?.pk) throw new Error("keys_test.json: no users.root.keys[0]");
    rootSk = r.sk; rootPk = r.pk;
}

const GREEN = "\x1b[32m"; const RED = "\x1b[31m"; const RESET = "\x1b[0m";
let pass = 0; let fail = 0;
function check(cond: boolean, msg: string) {
    if (cond) { pass++; console.log("  " + GREEN + "✅" + RESET + " " + msg); }
    else      { fail++; console.log("  " + RED   + "❌" + RESET + " " + msg); }
}
function errStr(e: any): string {
    return (e && typeof e === "object" && ("errcode" in e || "errstr" in e))
        ? "errcode=" + e.errcode + " errstr=" + e.errstr : String(e);
}

const pending: { [h: string]: { resolve: (b: Block) => void; reject: (e: any) => void; timer: any } } = {};
const inFlight = new Set<string>();

for (let i = 0; i < NODES; i++) {
    const na = nodeAddr(i);
    mtjs.tx_subscribe(na, (b: Block) => {
        for (const h of b.tx_hashes) {
            if (inFlight.has(h)) inFlight.delete(h);
            const p = pending[h];
            if (p) { delete pending[h]; clearTimeout(p.timer); p.resolve(b); }
        }
    });
}

function waitTx(h: string, to: number): Promise<Block> {
    return new Promise<Block>((res, rej) => {
        const timer = setTimeout(() => { delete pending[h]; rej({errcode:1,errstr:"timeout"}); }, to*1000);
        pending[h] = { resolve: res, reject: rej, timer };
    });
}

// ============================================================
// getNonceWithRetry — fallback на все ноды (sync может отставать)
// ============================================================
async function getNonceWithRetry(addr: string, preferredNode: string, retries = 20): Promise<bigint> {
    for (let a = 0; a < retries; a++) {
        const nodes = [preferredNode];
        for (let i = 0; i < NODES; i++) {
            const na = nodeAddr(i);
            if (na !== preferredNode) nodes.push(na);
        }
        for (const na of nodes) {
            try {
                const ui = await mtjs.get_user_nonce(na, addr, 3);
                if (ui.found) return BigInt(ui.nonce);
            } catch (e) {}
        }
        await new Promise(r => setTimeout(r, 500));
    }
    throw new Error("getNonceWithRetry: address not found: " + addr);
}

async function submitAndWait(sk: string, addr: string, tx: any[], nonce: string, value: number, nodeaddr: string, gasLimit = GAS_LIMIT, retries = 5): Promise<any> {
    for (let a = 0; a < retries; a++) {
        const req = { tx, nonce, value, gasLimit, priority: 0, sender: addr, key_id: 0 };
        const rsp = await mtjs.tx_submit(nodeaddr, 1, JSON.stringify(req), sk, nonce, () => {});
        if (rsp.err) {
            if (a+1 < retries) { await new Promise(r=>setTimeout(r,300)); continue; }
            throw new Error("submit: " + rsp.err_str);
        }
        const b = await waitTx(rsp.tx_hash, 20);
        const st = b.status(rsp.tx_hash);
        const es = b.errstr(rsp.tx_hash) || "";
        if (st === "err" && es.indexOf("invalid nonce") >= 0) {
            if (a+1 < retries) { await new Promise(r=>setTimeout(r,300)); continue; }
            throw new Error("invalid nonce: " + es);
        }
        return { rsp, block: b };
    }
    throw new Error("submitAndWait: retries exhausted");
}

interface BatchTx {
    sk: string;
    addr: string;
    tx: any[];
    nonce: string;
    value: number;
    node: string;
}

async function submitBatch(txs: BatchTx[]): Promise<number> {
    let sent = 0;
    const promises: Promise<void>[] = [];
    for (const t of txs) {
        const p = mtjs.tx_submit(t.node, 1, JSON.stringify({
            tx: t.tx, nonce: t.nonce, value: t.value, gasLimit: GAS_LIMIT,
            priority: 0, sender: t.addr, key_id: 0,
        }), t.sk, t.nonce, () => {})
            .then(rsp => {
                if (rsp.err) return;
                inFlight.add(rsp.tx_hash);
                sent++;
            })
            .catch(e => {});
        promises.push(p);
    }
    await Promise.all(promises);

    const t0 = Date.now();
    while (inFlight.size > 0) {
        await new Promise(r => setTimeout(r, 100));
        if (Date.now() - t0 > 120000) {
            console.log("  batch timeout: inFlight=" + inFlight.size);
            inFlight.clear();
            break;
        }
    }
    return sent;
}

const rootAddr = mtjs.addr_from_name("root");

async function ensureRootFunded(minBalance: bigint) {
    const ui = await mtjs.get_user_nonce(nodeAddr(0), rootAddr, 3);
    if (BigInt(ui.balance) >= minBalance) return;
    const nonce = (await getNonceWithRetry(rootAddr, nodeAddr(0))).toString();
    await submitAndWait(rootSk, rootAddr, [
        { contract: "root", method: "mint", params: { amount: "1000000000000000" } },
    ], nonce, 0, nodeAddr(0), 20000);
}

async function ensureTickets(src: string) {
    const c = await mtjs.get_contract(nodeAddr(0), "tickets", 3);
    const method = c.found ? "contract_update" : "contract_deploy";
    const nonce = (await getNonceWithRetry(rootAddr, nodeAddr(0))).toString();
    await submitAndWait(rootSk, rootAddr, [
        { contract: "root", method, params: { name: "tickets", src, ttl: 1000000 } },
    ], nonce, 0, nodeAddr(0), 20000);
}

// ============================================================
// processUser — 1000 билетов от одного юзера (10 батчей по 100)
// ============================================================
async function processUser(
    u: { name: string, addr: string, sk: string, node: string },
    ui: number,
    EVENT_NAME: string,
    progress: { sent: number, usersDone: number, t0: number }
): Promise<void> {
    const baseNonce = await getNonceWithRetry(u.addr, u.node);

    for (let bstart = 0; bstart < TICKETS_PER_USER; bstart += BATCH) {
        const bend = Math.min(bstart + BATCH, TICKETS_PER_USER);
        const batchTxs: BatchTx[] = [];
        for (let ti = bstart; ti < bend; ti++) {
            const gIdx = ui * TICKETS_PER_USER + ti;
            const sector = Math.floor(gIdx / 10000) % 10;
            const row = Math.floor(gIdx / 100) % 100;
            const seat = gIdx % 100;
            const nonce = (baseNonce + BigInt(ti)).toString();
            const tx = [{ contract: "tickets", method: "buyTicket", params: {
                event: EVENT_NAME, sector, row, seat, amount: 10,
            } }];
            batchTxs.push({ sk: u.sk, addr: u.addr, tx, nonce, value: 10, node: u.node });
        }
        const sent = await submitBatch(batchTxs);
        progress.sent += sent;
    }
    progress.usersDone++;
    if (progress.usersDone % 10 === 0) {
        const elapsed = (Date.now() - progress.t0) / 1000;
        console.log("  progress: users=" + progress.usersDone + "/" + USERS +
                    " sent=" + progress.sent + "/" + (USERS * TICKETS_PER_USER) +
                    " elapsed=" + elapsed.toFixed(0) + "s" +
                    " tps=" + (progress.sent / elapsed).toFixed(0));
    }
}

async function main() {
    await loadKeys();
    console.log("=== TICKETS 100k (users × " + USERS + ", tickets/user × " + TICKETS_PER_USER + ", batch=" + BATCH + ", userConcurrency=" + USER_CONCURRENCY + ") ===");
    await ensureRootFunded(1_000_000_000_000_000n);

    const TICKETS_SRC = "/home/bsr/mtjs/src/js/tsi/dist/contracts/tickets.js";
    const src = await mtjs.fs.readFile(TICKETS_SRC);
    check(src.length > 0, "tickets src read (" + src.length + " bytes)");
    await ensureTickets(src);

    const EVENT_NAME = "AC/DC_" + Date.now();
    console.log("  event name:", EVENT_NAME);

    // ============================================================
    // [t1] createEvent
    // ============================================================
    console.log("\n[t1] createEvent 10×100×100 = 100k мест");
    {
        const nonce = (await getNonceWithRetry(rootAddr, nodeAddr(0))).toString();
        const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
            { contract: "tickets", method: "createEvent", params: {
                name: EVENT_NAME, sectors: 10, rows: 100, seats: 100, price: 10,
            } },
        ], nonce, 0, nodeAddr(0), 20000);
        check(block.status(rsp.tx_hash) === "ok", "createEvent ok (errstr=" + (block.errstr(rsp.tx_hash)||"") + ")");
    }

    // ============================================================
    // [t2] getEvent
    // ============================================================
    console.log("\n[t2] getEvent");
    {
        const r = await mtjs.call_view(nodeAddr(0), "tickets", "getEvent", { event: EVENT_NAME }, 10, 10);
        check(r.found === true && !r.error, "view getEvent ok");
        if (r.found && !r.error) {
            const v = JSON.parse(r.result);
            check(v.total === 100000, "total=100k (got " + v.total + ")");
            check(v.sold === 0, "sold=0");
        }
    }

    // ============================================================
    // [t3] 100 юзеров (uu0..uu99) — параллельно (create + transfer), ЖДЁМ inFlight
    // ============================================================
    console.log("\n[t3] generate " + USERS + " users (uu0..uu" + (USERS-1) + ") + fund (21M each) — parallel");
    const users: { name: string, addr: string, sk: string, node: string }[] = [];
    const TS = Date.now();
    const need = BigInt(TICKETS_PER_USER) * BigInt(GAS_LIMIT) + 1000000n;

    const rootBaseNonce = await getNonceWithRetry(rootAddr, nodeAddr(0));

    interface UserSubmit {
        ok: boolean;
        rsp?: any;
        err?: any;
        name: string;
        phase: string;
    }
    const promises: Promise<UserSubmit>[] = [];

    for (let ui = 0; ui < USERS; ui++) {
        const uName = "uu" + ui;
        const uAddr = mtjs.blake2b_hex("uu_gen_" + TS + "_" + ui);
        const na = nodeAddr(ui);
        const kp = mtjs.generate_keypair(0);
        const nonceCreate = (rootBaseNonce + BigInt(ui * 2)).toString();
        const nonceTransfer = (rootBaseNonce + BigInt(ui * 2 + 1)).toString();

        promises.push(
            mtjs.tx_submit(na, 1, JSON.stringify({
                tx: [{ contract: "root", method: "create_address", params: {
                    addr: uAddr,
                    keys: [{ type: 0, keybuffer: kp.pk }],
                    ttl: 10000000,
                } }],
                nonce: nonceCreate, value: 0, gasLimit: 20000, priority: 0, sender: rootAddr, key_id: 0,
            }), rootSk, nonceCreate, () => {})
            .then((rsp): UserSubmit => {
                if (!rsp.err) inFlight.add(rsp.tx_hash);   // ← добавляем в inFlight
                return { ok: !rsp.err, rsp, name: uName, phase: "create" };
            })
            .catch((e): UserSubmit => ({ ok: false, err: e, name: uName, phase: "create" }))
        );

        promises.push(
            mtjs.tx_submit(na, 1, JSON.stringify({
                tx: [{ contract: "root", method: "transfer", params: {
                    to: uAddr, amount: need.toString(),
                } }],
                nonce: nonceTransfer, value: Number(need), gasLimit: 20000, priority: 0, sender: rootAddr, key_id: 0,
            }), rootSk, nonceTransfer, () => {})
            .then((rsp): UserSubmit => {
                if (!rsp.err) inFlight.add(rsp.tx_hash);   // ← добавляем в inFlight
                return { ok: !rsp.err, rsp, name: uName, phase: "transfer" };
            })
            .catch((e): UserSubmit => ({ ok: false, err: e, name: uName, phase: "transfer" }))
        );

        users.push({ name: uName, addr: uAddr, sk: kp.sk, node: na });
    }

    console.log("  submitted 200 TX (create + transfer), waiting... inFlight=" + inFlight.size);
    const results: UserSubmit[] = await Promise.all(promises);
    const okCount = results.filter(r => r.ok).length;
    console.log("  submit ok: " + okCount + "/" + results.length + ", inFlight=" + inFlight.size);

    const tw = Date.now();
    while (inFlight.size > 0) {
        await new Promise(r => setTimeout(r, 200));
        if (Date.now() - tw > 120000) { console.log("  t3 timeout: inFlight=" + inFlight.size); inFlight.clear(); break; }
    }
    console.log("  all users funded in " + ((Date.now()-tw)/1000).toFixed(0) + "s");
    await new Promise(r => setTimeout(r, 2000));   // ждать sync
    check(users.length === USERS, "users created: " + users.length + "/" + USERS);

    // проверка, что адреса видны (на всякий случай)
    let visible = 0;
    for (const u of users) {
        try { await getNonceWithRetry(u.addr, u.node, 2); visible++; } catch (e) {}
    }
    console.log("  users visible: " + visible + "/" + USERS);

    // ============================================================
    // [t4] buyTicket — параллельно по USER_CONCURRENCY юзеров
    // ============================================================
    console.log("\n[t4] buyTicket × " + (USERS * TICKETS_PER_USER) + " (batch=" + BATCH + ", userConcurrency=" + USER_CONCURRENCY + ")");
    const t0 = Date.now();
    const progress = { sent: 0, usersDone: 0, t0 };

    for (let start = 0; start < users.length; start += USER_CONCURRENCY) {
        const batch = users.slice(start, start + USER_CONCURRENCY);
        const userPromises: Promise<void>[] = [];
        for (let i = 0; i < batch.length; i++) {
            userPromises.push(
                processUser(batch[i], start + i, EVENT_NAME, progress)
                    .catch(e => console.log("  user " + batch[i].name + " EXC: " + errStr(e)))
            );
        }
        await Promise.all(userPromises);
        console.log("  === users " + (start + batch.length) + "/" + users.length +
                    " done, sent=" + progress.sent + " elapsed=" + ((Date.now()-t0)/1000).toFixed(0) + "s");
    }

    const dt = Date.now() - t0;
    console.log("  totalSent=" + progress.sent + " in " + dt + "ms (" + (dt/1000).toFixed(0) + "s)" +
                " tps=" + (progress.sent / (dt/1000)).toFixed(0));

    // ============================================================
    // [t5] getEvent
    // ============================================================
    console.log("\n[t5] getEvent — sold=" + (USERS * TICKETS_PER_USER));
    {
        const r = await mtjs.call_view(nodeAddr(0), "tickets", "getEvent", { event: EVENT_NAME }, 60, 60);
        if (r.found && !r.error) {
            const v = JSON.parse(r.result);
            check(v.sold === USERS * TICKETS_PER_USER, "sold=" + (USERS * TICKETS_PER_USER) + " (got " + v.sold + ")");
        }
    }

    // ============================================================
    // [t6] getTicket
    // ============================================================
    console.log("\n[t6] getTicket uu0/sector0/row0/seat0");
    {
        const r = await mtjs.call_view(nodeAddr(0), "tickets", "getTicket",
            { event: EVENT_NAME, sector: 0, row: 0, seat: 0 }, 10, 10);
        if (r.found && !r.error) {
            const v = JSON.parse(r.result);
            check(v.found === true, "ticket found");
            check(v.sold === true, "sold=true");
        }
    }

    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0) console.log(GREEN + "DONE" + RESET);
    else            console.log(RED   + "FAIL" + RESET);
    std.exit(fail);
}

main().catch((e: any) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => {}, 1000000);
export {};
