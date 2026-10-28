#!/usr/local/bin/mtjs
import * as std from "std";
// chk-node-disable.ts — node_disable/enable + missed_rounds + min_stake.
// n0..n9 (b_run-nodes) + доп нода <name> (env NEW_NODE_NAME, :2310, k_node10_*).
// node :2301 (n1, основная) — не трогаем.
//
// T1: node_disable <name> → enabled=false, missed не растёт
// T2: node_enable already enabled → err
// T3: node_disable already disabled → err
// T4: node_disable not owner (u1) → err
// T5: node_unstake 150 → stake=0 → не валидатор → missed растёт
// T6: node_stake 150 → stake=150 → full_stake=150 (missed=0 НЕ проверяем — нужен ответ на HB)
// T7: node_unstake 60 → stake=90 < 100 → missed растёт
// T8: node_stake 10 → stake=100 (at min_stake) → full_stake=100 (missed=0 НЕ проверяем — нужен ответ на HB)
// T9: node_unstake 50 → stake=50 < 100 → missed растёт
// T10: node_unstake 50 → stake=0 → missed растёт
//
// missed_rounds — getMissedAny() (max по n0..n9), т.к. конкретная нода
// (n1) может быть в sync и не растить missed.
const node = "127.0.0.1:2301";
const rootSk = std.getenv("u_root_ed_sk") || std.getenv("k_root_ed_sk");
const rootPk = std.getenv("k_root_ed_pk");
if (!rootSk || !rootPk)
    throw new Error("no root sk/pk");
const u1sk = std.getenv("k_u1_ed_sk");
const u1pk = std.getenv("k_u1_ed_pk");
if (!u1sk || !u1pk)
    throw new Error("no k_u1_* keys");
const newNodeNameRaw = std.getenv("NEW_NODE_NAME");
if (!newNodeNameRaw)
    throw new Error("no NEW_NODE_NAME env");
const newNodeName = newNodeNameRaw;
const newNodeInst = "MTJS" + newNodeName;
const newNodeRpc = "127.0.0.1:2310";
const newNodeWebPort = 7710;
const newNodePk = std.getenv("k_node10_ed_pk");
const newNodeBls = std.getenv("k_node10_bls_pk");
if (!newNodePk || !newNodeBls)
    throw new Error("no k_node10_* keys");
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
const handles = {};
function nodeConf(name, rpcAddr, edKey, blsKey, dbPath, webPort) {
    return `
        Start=Node
            Node_rpc_addr=${rpcAddr}
            Node_web_addr=0.0.0.0:${webPort}
            Node_rockdb_path=${dbPath}
            MTJS_PENDING_TIMEOUT=0.2
            MTJS_STACK_SIZE=8388608
            MTJS_THREAD_POOL_SIZE=10
            mtjs_deviceName=Device
            SocketIO_listen_backlog=128
            SocketIO_size=1024
            SocketIO_epoll_timeout_millisec=2000
            SocketIO_n_workers=2
            Oscar_maxPacketSize=33554432
            HTTP_max_post=1000000
            HTTP_doc_urls=/pics,/html,/css
            HTTP_document_root=./www
            GrainWriter_snapshot_modulus=1000
            Node_my_sk_bls_env_key=${blsKey}
            Node_my_sk_ed_env_key=${edKey}
            Node_this_node_name=${name}
            Node_sqlite_pn=${dbPath}_s
            db_name=${dbPath}_db
            mysql_max_connections=20
            mysql_host=NULL
            mysql_user=root
            mysql_passwd=gaga123
            mysql_database=md
            mysql_port=0
            mysql_sock=/run/mysqld/mysqld.sock
    `;
}
function addNewNode() {
    const h = mtjs.addInstance(newNodeInst, nodeConf(newNodeName, newNodeRpc, "k_node10_ed_sk", "k_node10_bls_sk", `db/r_${newNodeName}`, newNodeWebPort));
    handles[newNodeInst] = h;
    console.log(`[addInstance] ${newNodeInst} (${newNodeName}) handle=${h} rpc=${newNodeRpc} web=:${newNodeWebPort}`);
    return h;
}
function killInstance(name) {
    const h = handles[name];
    if (!h) {
        console.log(`[killInstance] ${name} no handle`);
        return;
    }
    console.log(`[killInstance] ${name} handle=${h}`);
    mtjs.killInstance(h);
    delete handles[name];
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
    return BigInt(ui.nonce);
}
async function waitNonceGt(addr, lastNonce, timeoutSec = 10) {
    const t0 = Date.now();
    while (Date.now() - t0 < timeoutSec * 1000) {
        const n = await getNonce(addr);
        if (n > lastNonce)
            return n;
        await new Promise((r) => setTimeout(() => r(), 200));
    }
    throw new Error("waitNonceGt timeout: lastNonce=" + lastNonce.toString());
}
async function submitAndWait(sk, addr, tx, value = 0, gasLimit = 20000) {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    const req = { tx, nonce: ui.nonce, value, gasLimit, gasPrice: 10 };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, ui.nonce, () => { });
    if (rsp.err)
        throw new Error("submit: " + rsp.err_str);
    const b = await waitTx(rsp.tx_hash, 10);
    await waitNonceGt(addr, BigInt(ui.nonce), 10);
    return { rsp, block: b };
}
async function submitAndWaitErr(sk, addr, tx, value = 0, gasLimit = 20000) {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    const req = { tx, nonce: ui.nonce, value, gasLimit, gasPrice: 10 };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, ui.nonce, () => { });
    if (rsp.err)
        throw new Error("submit: " + rsp.err_str);
    const b = await waitTx(rsp.tx_hash, 10);
    await waitNonceGt(addr, BigInt(ui.nonce), 10);
    return { rsp, block: b };
}
const rootAddr = mtjs.addr_from_pk(rootPk);
const u1Addr = mtjs.addr_from_pk(u1pk);
const NODES = 10;
async function getMissedAny(name) {
    let maxMissed = 0;
    for (let i = 0; i < NODES; i++) {
        try {
            const ni = await mtjs.get_node_info(`127.0.0.1:${2300 + i}`, name, 3);
            if (ni.found && ni.missed_rounds > maxMissed)
                maxMissed = ni.missed_rounds;
        }
        catch (e) { }
    }
    return maxMissed;
}
async function fiveBlocks() {
    for (let i = 0; i < 5; i++) {
        await submitAndWait(u1sk, u1Addr, [
            { contract: "root", method: "transfer", params: { to: rootAddr, amount: "10" } },
        ], 1000000, 100000);
    }
}
async function setup() {
    console.log("\n[setup] ensure " + newNodeName);
    let up = false;
    for (let t = 0; t < 20; t++) {
        try {
            await mtjs.get_values(node, 1);
            up = true;
            break;
        }
        catch (e) {
            await new Promise((r) => setTimeout(() => r(), 500));
        }
    }
    check(up, "node :2301 up (get_values)");
    let ni = await mtjs.get_node_info(node, newNodeName, 3);
    if (!ni.found) {
        console.log("  " + newNodeName + " not found — creating...");
        await submitAndWait(rootSk, rootAddr, [
            { contract: "root", method: "node_create", params: {
                    name: newNodeName, ip: newNodeRpc,
                    pk_ed: newNodePk, pk_bls: newNodeBls,
                } },
        ], 0, 1000000);
        ni = await mtjs.get_node_info(node, newNodeName, 3);
    }
    if (BigInt(ni.full_stake) === 0n) {
        console.log("  " + newNodeName + " stake=0 — staking 150...");
        await submitAndWait(u1sk, u1Addr, [
            { contract: "root", method: "node_stake", params: { node: newNodeName, amount: "150" } },
        ], 1000, 100000);
        ni = await mtjs.get_node_info(node, newNodeName, 3);
    }
    check(ni.found === true, newNodeName + " found");
    check(ni.enabled === true, newNodeName + " enabled=true");
    check(BigInt(ni.full_stake) === 150n, newNodeName + " full_stake=150");
    // поднять доп-ноду (для T6/T8 stake/full_stake; missed=0 не гарантирован — sync)
    addNewNode();
    await new Promise((r) => setTimeout(() => r(), 1500));
    const ni2 = await mtjs.get_node_info(node, newNodeName, 3);
    console.log("  " + newNodeName + " after addInstance:", JSON.stringify({ found: ni2.found, enabled: ni2.enabled, full_stake: ni2.full_stake }));
}
async function t1_disable() {
    console.log("\n[T1] node_disable " + newNodeName + " → enabled=false, missed not growing");
    const ni0 = await mtjs.get_node_info(node, newNodeName, 3);
    check(ni0.enabled === true, newNodeName + " enabled=true (before)");
    const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
        { contract: "root", method: "node_disable", params: { node: newNodeName } },
    ]);
    check(block.status(rsp.tx_hash) === "ok", "node_disable ok");
    const ni1 = await mtjs.get_node_info(node, newNodeName, 3);
    check(ni1.enabled === false, newNodeName + " enabled=false (after)");
    const mr0 = await getMissedAny(newNodeName);
    await fiveBlocks();
    const ni2 = await mtjs.get_node_info(node, newNodeName, 3);
    check(ni2.enabled === false, newNodeName + " still enabled=false");
    const mr1 = await getMissedAny(newNodeName);
    check(mr1 === mr0, newNodeName + " missed not growing (disabled): " + mr0 + " -> " + mr1);
    const { rsp: rsp2, block: block2 } = await submitAndWait(rootSk, rootAddr, [
        { contract: "root", method: "node_enable", params: { node: newNodeName } },
    ]);
    check(block2.status(rsp2.tx_hash) === "ok", "node_enable ok");
    const ni3 = await mtjs.get_node_info(node, newNodeName, 3);
    check(ni3.enabled === true, newNodeName + " enabled=true (after enable)");
}
async function t2_enable_already() {
    console.log("\n[T2] node_enable already enabled → err");
    const { rsp, block } = await submitAndWaitErr(rootSk, rootAddr, [
        { contract: "root", method: "node_enable", params: { node: newNodeName } },
    ]);
    check(block.status(rsp.tx_hash) === "err", "node_enable already enabled err");
    check((block.errstr(rsp.tx_hash) || "").indexOf("already enabled") >= 0, "errstr already enabled");
}
async function t3_disable_already() {
    console.log("\n[T3] node_disable already disabled → err");
    await submitAndWait(rootSk, rootAddr, [
        { contract: "root", method: "node_disable", params: { node: newNodeName } },
    ]);
    const { rsp, block } = await submitAndWaitErr(rootSk, rootAddr, [
        { contract: "root", method: "node_disable", params: { node: newNodeName } },
    ]);
    check(block.status(rsp.tx_hash) === "err", "node_disable already disabled err");
    check((block.errstr(rsp.tx_hash) || "").indexOf("already disabled") >= 0, "errstr already disabled");
    await submitAndWait(rootSk, rootAddr, [
        { contract: "root", method: "node_enable", params: { node: newNodeName } },
    ]);
}
async function t4_disable_not_owner() {
    console.log("\n[T4] node_disable not owner (u1) → err");
    const { rsp, block } = await submitAndWaitErr(u1sk, u1Addr, [
        { contract: "root", method: "node_disable", params: { node: newNodeName } },
    ], 1000000, 100000);
    check(block.status(rsp.tx_hash) === "err", "node_disable not owner err");
    check((block.errstr(rsp.tx_hash) || "").indexOf("owner") >= 0, "errstr owner");
}
async function t5_unstake_zero() {
    console.log("\n[T5] node_unstake 150 → stake=0 → не валидатор → missed растёт");
    const { rsp, block } = await submitAndWait(u1sk, u1Addr, [
        { contract: "root", method: "node_unstake", params: { node: newNodeName, amount: "150" } },
    ], 0, 100000);
    check(block.status(rsp.tx_hash) === "ok", "node_unstake 150 ok");
    const ni0 = await mtjs.get_node_info(node, newNodeName, 3);
    check(BigInt(ni0.full_stake) === 0n, newNodeName + " full_stake=0");
    const mr0 = await getMissedAny(newNodeName);
    await fiveBlocks();
    const mr1 = await getMissedAny(newNodeName);
    check(mr1 > mr0, newNodeName + " missed grows (stake=0): " + mr0 + " -> " + mr1);
}
async function t6_stake_restore() {
    console.log("\n[T6] node_stake 150 → stake=150 (validator)");
    const { rsp, block } = await submitAndWait(u1sk, u1Addr, [
        { contract: "root", method: "node_stake", params: { node: newNodeName, amount: "150" } },
    ], 1000, 100000);
    check(block.status(rsp.tx_hash) === "ok", "node_stake 150 ok");
    const ni0 = await mtjs.get_node_info(node, newNodeName, 3);
    check(BigInt(ni0.full_stake) === 150n, newNodeName + " full_stake=150");
    // missed=0 НЕ проверяем: сброс только если нода ответила на HB
    // (в LeaderCert.node_validators), а новая доп-нода может быть в sync.
    await fiveBlocks();
    const mr = await getMissedAny(newNodeName);
    console.log("  [diag] " + newNodeName + " missed=" + mr + " (informational, not checked)");
}
async function t7_unstake_below_min() {
    console.log("\n[T7] node_unstake 60 → stake=90 < 100 → missed растёт");
    const { rsp, block } = await submitAndWait(u1sk, u1Addr, [
        { contract: "root", method: "node_unstake", params: { node: newNodeName, amount: "60" } },
    ], 0, 100000);
    check(block.status(rsp.tx_hash) === "ok", "node_unstake 60 ok");
    const ni0 = await mtjs.get_node_info(node, newNodeName, 3);
    check(BigInt(ni0.full_stake) === 90n, newNodeName + " full_stake=90");
    const mr0 = await getMissedAny(newNodeName);
    await fiveBlocks();
    const mr1 = await getMissedAny(newNodeName);
    check(mr1 > mr0, newNodeName + " missed grows (stake=90 < 100): " + mr0 + " -> " + mr1);
}
async function t8_stake_at_min() {
    console.log("\n[T8] node_stake 10 → stake=100 (at min_stake)");
    const { rsp, block } = await submitAndWait(u1sk, u1Addr, [
        { contract: "root", method: "node_stake", params: { node: newNodeName, amount: "10" } },
    ], 1000, 100000);
    check(block.status(rsp.tx_hash) === "ok", "node_stake 10 ok");
    const ni0 = await mtjs.get_node_info(node, newNodeName, 3);
    check(BigInt(ni0.full_stake) === 100n, newNodeName + " full_stake=100 (at min_stake)");
    // missed=0 НЕ проверяем (как T6).
    const mr = await getMissedAny(newNodeName);
    console.log("  [diag] " + newNodeName + " missed=" + mr + " (informational, not checked)");
}
async function t9_unstake_50() {
    console.log("\n[T9] node_unstake 50 → stake=50 < 100 → missed растёт");
    const { rsp, block } = await submitAndWait(u1sk, u1Addr, [
        { contract: "root", method: "node_unstake", params: { node: newNodeName, amount: "50" } },
    ], 0, 100000);
    check(block.status(rsp.tx_hash) === "ok", "node_unstake 50 ok");
    const ni0 = await mtjs.get_node_info(node, newNodeName, 3);
    check(BigInt(ni0.full_stake) === 50n, newNodeName + " full_stake=50");
    const mr0 = await getMissedAny(newNodeName);
    await fiveBlocks();
    const mr1 = await getMissedAny(newNodeName);
    check(mr1 > mr0, newNodeName + " missed grows (stake=50 < 100): " + mr0 + " -> " + mr1);
}
async function t10_unstake_zero() {
    console.log("\n[T10] node_unstake 50 → stake=0 → missed растёт");
    const { rsp, block } = await submitAndWait(u1sk, u1Addr, [
        { contract: "root", method: "node_unstake", params: { node: newNodeName, amount: "50" } },
    ], 0, 100000);
    check(block.status(rsp.tx_hash) === "ok", "node_unstake 50 ok");
    const ni0 = await mtjs.get_node_info(node, newNodeName, 3);
    check(BigInt(ni0.full_stake) === 0n, newNodeName + " full_stake=0");
    const mr0 = await getMissedAny(newNodeName);
    await fiveBlocks();
    const mr1 = await getMissedAny(newNodeName);
    check(mr1 > mr0, newNodeName + " missed grows (stake=0): " + mr0 + " -> " + mr1);
}
(async () => {
    console.log("=== NODE DISABLE/ENABLE/STAKE (new node " + newNodeName + ") ===");
    await setup();
    await t1_disable();
    await t2_enable_already();
    await t3_disable_already();
    await t4_disable_not_owner();
    await t5_unstake_zero();
    await t6_stake_restore();
    await t7_unstake_below_min();
    await t8_stake_at_min();
    await t9_unstake_50();
    await t10_unstake_zero();
    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0)
        console.log(GREEN + "DONE" + RESET);
    else
        console.log(RED + "FAIL" + RESET);
})().catch((e) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => { }, 1000000);
