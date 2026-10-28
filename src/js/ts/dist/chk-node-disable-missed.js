#!/usr/local/bin/mtjs
import * as std from "std";
// chk-node-disable-missed.ts — missed_rounds: сброс на node_enable (setEnabled),
// рост при stake < min_stake. n0..n9 (b_run-nodes) + доп <name> (env NEW_NODE_NAME).
// node :2301 (n1, основная) — не трогаем.
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
    addNewNode();
    await new Promise((r) => setTimeout(() => r(), 1500));
}
// T1: node_enable → missed=0 (setEnabled(true) сброс)
async function t1_enable_reset() {
    console.log("\n[T1] node_disable → node_enable → missed=0 (setEnabled(true) сброс)");
    await submitAndWait(rootSk, rootAddr, [
        { contract: "root", method: "node_disable", params: { node: newNodeName } },
    ]);
    const mr0 = await getMissedAny(newNodeName);
    console.log("  missed before enable:", mr0);
    await fiveBlocks(); // disabled — missed не растёт
    const mr1 = await getMissedAny(newNodeName);
    check(mr1 === mr0, newNodeName + " missed not growing (disabled): " + mr0 + " -> " + mr1);
    const { rsp, block } = await submitAndWait(rootSk, rootAddr, [
        { contract: "root", method: "node_enable", params: { node: newNodeName } },
    ]);
    check(block.status(rsp.tx_hash) === "ok", "node_enable ok");
    const ni = await mtjs.get_node_info(node, newNodeName, 3);
    check(ni.enabled === true, newNodeName + " enabled=true");
    const mr2 = await getMissedAny(newNodeName);
    check(mr2 === 0, newNodeName + " missed=0 after node_enable (setEnabled reset), got " + mr2);
}
// T2: stake=0 → не валидатор → missed растёт
async function t2_unstake_zero() {
    console.log("\n[T2] node_unstake 150 → stake=0 → missed растёт");
    await submitAndWait(u1sk, u1Addr, [
        { contract: "root", method: "node_unstake", params: { node: newNodeName, amount: "150" } },
    ], 0, 100000);
    const ni0 = await mtjs.get_node_info(node, newNodeName, 3);
    check(BigInt(ni0.full_stake) === 0n, newNodeName + " full_stake=0");
    const mr0 = await getMissedAny(newNodeName);
    await fiveBlocks();
    const mr1 = await getMissedAny(newNodeName);
    check(mr1 > mr0, newNodeName + " missed grows (stake=0): " + mr0 + " -> " + mr1);
}
// T3: stake=90 < 100 → не валидатор → missed растёт
async function t3_stake_below_min() {
    console.log("\n[T3] node_stake 90 → stake=90 < 100 → missed растёт");
    await submitAndWait(u1sk, u1Addr, [
        { contract: "root", method: "node_stake", params: { node: newNodeName, amount: "90" } },
    ], 1000, 100000);
    const ni0 = await mtjs.get_node_info(node, newNodeName, 3);
    check(BigInt(ni0.full_stake) === 90n, newNodeName + " full_stake=90");
    const mr0 = await getMissedAny(newNodeName);
    await fiveBlocks();
    const mr1 = await getMissedAny(newNodeName);
    check(mr1 > mr0, newNodeName + " missed grows (stake=90 < 100): " + mr0 + " -> " + mr1);
}
// T4: stake=50 < 100 → missed растёт
async function t4_stake_50() {
    console.log("\n[T4] node_unstake 40 → stake=50 < 100 → missed растёт");
    await submitAndWait(u1sk, u1Addr, [
        { contract: "root", method: "node_unstake", params: { node: newNodeName, amount: "40" } },
    ], 0, 100000);
    const ni0 = await mtjs.get_node_info(node, newNodeName, 3);
    check(BigInt(ni0.full_stake) === 50n, newNodeName + " full_stake=50");
    const mr0 = await getMissedAny(newNodeName);
    await fiveBlocks();
    const mr1 = await getMissedAny(newNodeName);
    check(mr1 > mr0, newNodeName + " missed grows (stake=50 < 100): " + mr0 + " -> " + mr1);
}
// T5: stake=0 → missed растёт
async function t5_unstake_zero() {
    console.log("\n[T5] node_unstake 50 → stake=0 → missed растёт");
    await submitAndWait(u1sk, u1Addr, [
        { contract: "root", method: "node_unstake", params: { node: newNodeName, amount: "50" } },
    ], 0, 100000);
    const ni0 = await mtjs.get_node_info(node, newNodeName, 3);
    check(BigInt(ni0.full_stake) === 0n, newNodeName + " full_stake=0");
    const mr0 = await getMissedAny(newNodeName);
    await fiveBlocks();
    const mr1 = await getMissedAny(newNodeName);
    check(mr1 > mr0, newNodeName + " missed grows (stake=0): " + mr0 + " -> " + mr1);
}
(async () => {
    console.log("=== NODE DISABLE/MISSED TESTS (new node " + newNodeName + ") ===");
    await setup();
    await t1_enable_reset();
    await t2_unstake_zero();
    await t3_stake_below_min();
    await t4_stake_50();
    await t5_unstake_zero();
    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0)
        console.log(GREEN + "DONE" + RESET);
    else
        console.log(RED + "FAIL" + RESET);
})().catch((e) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => { }, 1000000);
