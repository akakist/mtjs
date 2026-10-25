#!/usr/local/bin/mtjs
import * as std from "std";
// ============================================================
// b_test-node-load-pause.ts
// 20 юзеров, каждый: пачка TX -> ждём блок -> иногда пауза -> снова.
// Ловит залипание консенсуса после паузы (TIMER_RESTART_BLOCK и т.п.).
// ============================================================
const node = "127.0.0.1:2301";
function envInt(name, def) {
    const v = std.getenv(name);
    if (!v)
        return def;
    const n = Number(v);
    return (isFinite(n) && n > 0) ? n : def;
}
function envStr(name, def) {
    const v = std.getenv(name);
    return (v && v.length > 0) ? v : def;
}
const USERS = envInt("USERS", 20);
const ITERS = envInt("ITERS", 30);
const PAUSE_EVERY = envInt("PAUSE_EVERY", 5);
const PAUSE_SEC = envInt("PAUSE_SEC", 15);
const GAS_PRICE = envStr("GAS_PRICE", "1");
const GAS_LIMIT = envStr("GAS_LIMIT", "200000");
const MINT_AMOUNT = envStr("MINT_AMOUNT", "1000000");
const THROTTLE_MS = envInt("THROTTLE_MS", 100);
const WAIT_TX_SEC = envInt("WAIT_TX_SEC", 20);
const WAIT_NONCE_SEC = envInt("WAIT_NONCE_SEC", 15);
const WATCHDOG_MS = 10000;
const GREEN = "\x1b[32m";
const RED = "\x1b[31m";
const RESET = "\x1b[0m";
let pass = 0, fail = 0;
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
function sleep(ms) { return new Promise((r) => setTimeout(() => r(), ms)); }
const users = [];
for (let i = 0; i < USERS; i++) {
    const sk = std.getenv("k_u" + i + "_ed_sk");
    const pk = std.getenv("k_u" + i + "_ed_pk");
    if (!sk || !pk) {
        console.log("skip u" + i + ": no keys");
        continue;
    }
    users.push({ name: "u" + i, sk, pk, addr: mtjs.addr_from_pk(pk) });
}
if (users.length === 0) {
    console.log(RED + "no users" + RESET);
}
// ---------- pending tx -> block ----------
const pending = {};
let lastBlockTs = 0;
let lastTxSubmitTs = 0;
let firstTxTs = 0;
let blocks = 0;
let maxGapMs = 0;
mtjs.tx_subscribe(node, (b) => {
    const now = Date.now();
    if (lastBlockTs) {
        const g = now - lastBlockTs;
        if (g > maxGapMs)
            maxGapMs = g;
    }
    lastBlockTs = now;
    blocks++;
    for (const h of b.tx_hashes) {
        const p = pending[h];
        if (p) {
            delete pending[h];
            clearTimeout(p.timer);
            p.resolve(b);
        }
    }
});
function waitTx(h, toSec) {
    return new Promise((res, rej) => {
        const timer = setTimeout(() => {
            delete pending[h];
            rej({ errcode: 1, errstr: "timed out (no block with tx " + h.slice(0, 8) + ")" });
        }, toSec * 1000);
        pending[h] = { resolve: res, reject: rej, timer };
    });
}
async function getNonce(addr) {
    const ui = await mtjs.get_user_nonce(node, addr, 5);
    return BigInt(ui.nonce);
}
async function waitNonceGt(addr, lastNonce, timeoutSec) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeoutSec * 1000) {
        const n = await getNonce(addr);
        if (n > lastNonce)
            return n;
        await sleep(200);
    }
    throw new Error("waitNonceGt timeout last=" + lastNonce.toString());
}
// пачка TX: submit -> waitTx -> waitNonceGt
async function submitBatch(u, tx) {
    const nonce = await getNonce(u.addr);
    const req = { tx, nonce: nonce.toString(), value: 1000000, gasLimit: Number(GAS_LIMIT), gasPrice: Number(GAS_PRICE) };
    const now = Date.now();
    lastTxSubmitTs = now;
    if (!firstTxTs)
        firstTxTs = now;
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), u.sk, nonce.toString(), () => { });
    if (rsp.err)
        throw new Error("submit: " + rsp.err_str);
    const b = await waitTx(rsp.tx_hash, WAIT_TX_SEC);
    await waitNonceGt(u.addr, nonce, WAIT_NONCE_SEC);
    return { rsp, block: b, nonce };
}
function childEmits(b, txHash) {
    const ch = b.children(txHash);
    const out = {};
    if (ch)
        for (const k of Object.keys(ch))
            out[k] = ch[k].emits;
    return out;
}
function hasEmit(em, name) {
    for (const k of Object.keys(em))
        for (const e of em[k])
            if (e[0] === name)
                return e[1];
    return null;
}
// ---------- сторож ----------
let watchdogFailed = false;
const watchdog = setInterval(() => {
    const now = Date.now();
    if (lastTxSubmitTs === 0)
        return;
    const sinceLastTx = now - lastTxSubmitTs;
    const sinceLastBlockOrFirst = lastBlockTs > 0
        ? (now - lastBlockTs)
        : (now - (firstTxTs || lastTxSubmitTs));
    if (sinceLastTx < WATCHDOG_MS && sinceLastBlockOrFirst > WATCHDOG_MS && !watchdogFailed) {
        watchdogFailed = true;
        console.log(RED + "  ❌ WATCHDOG: last TX " + sinceLastTx + "ms ago, no block for " + sinceLastBlockOrFirst + "ms" + RESET);
        fail++;
    }
}, 1000);
// ---------- цикл одного юзера ----------
async function userLoop(u, ui) {
    const other = users[(ui + 1) % users.length];
    const other2 = users[(ui + 2) % users.length];
    const nk = "n" + (ui % 10);
    for (let iter = 0; iter < ITERS; iter++) {
        const tag = "[" + u.name + "/i" + iter + "]";
        let tx;
        if (iter === 0) {
            tx = [
                { contract: "root", method: "mint", params: { amount: MINT_AMOUNT } },
                { contract: "root", method: "transfer", params: { to: other.addr, amount: "100" } },
                { contract: "root", method: "node_stake", params: { node: nk, amount: "100" } },
                { contract: "root", method: "node_unstake", params: { node: nk, amount: "50" } },
            ];
        }
        else {
            tx = [
                { contract: "root", method: "transfer", params: { to: other.addr, amount: "100" } },
                { contract: "root", method: "transfer", params: { to: other2.addr, amount: "50" } },
                { contract: "root", method: "node_stake", params: { node: nk, amount: "100" } },
                { contract: "root", method: "node_unstake", params: { node: nk, amount: "50" } },
            ];
        }
        try {
            const { rsp, block } = await submitBatch(u, tx);
            check(block.status(rsp.tx_hash) === "ok", tag + " batch ok");
            if (iter === 0) {
                check(hasEmit(childEmits(block, rsp.tx_hash), "mint") !== null, tag + " mint emit");
            }
        }
        catch (e) {
            check(false, tag + " batch: " + errStr(e));
        }
        if ((iter + 1) % PAUSE_EVERY === 0 && iter + 1 < ITERS) {
            console.log("  " + tag + " pause " + PAUSE_SEC + "s...");
            await sleep(PAUSE_SEC * 1000);
        }
        else {
            await sleep(THROTTLE_MS);
        }
    }
}
// ---------- main ----------
(async () => {
    console.log("=== B_TEST-NODE-LOAD-PAUSE ===");
    console.log("users=" + users.length + " iters=" + ITERS + " pauseEvery=" + PAUSE_EVERY + " pauseSec=" + PAUSE_SEC +
        " gasPrice=" + GAS_PRICE + " gasLimit=" + GAS_LIMIT + " throttle=" + THROTTLE_MS + "ms");
    if (users.length === 0) {
        console.log(RED + "no users" + RESET);
        return;
    }
    await sleep(500);
    const t0 = Date.now();
    await Promise.all(users.map((u, i) => userLoop(u, i).catch((e) => check(false, "[u" + i + "] EXC: " + errStr(e)))));
    const dt = Date.now() - t0;
    clearInterval(watchdog);
    console.log("\n--- summary ---");
    console.log("  duration=" + dt + "ms blocks=" + blocks + " maxGapMs=" + maxGapMs);
    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0 && !watchdogFailed)
        console.log(GREEN + "DONE" + RESET);
    else
        console.log(RED + "FAIL" + RESET);
})().catch((e) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => { }, 1000000);
