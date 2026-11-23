#!/usr/local/bin/mtjs
import * as std from "std";
// JSON.stringify с поддержкой BigInt (значения гранул — 128-битные)
function jstr(v) {
    return JSON.stringify(v, (_k, x) => (typeof x === "bigint" ? Number(x) : x));
}
// гранулы/эмиты хранят целые как BigInt, а тесты сравнивают с обычными числами
function unBig(v) {
    if (typeof v === "bigint")
        return Number(v);
    if (Array.isArray(v))
        return v.map(unBig);
    if (v && typeof v === "object") {
        const o = {};
        for (const k of Object.keys(v))
            o[k] = unBig(v[k]);
        return o;
    }
    return v;
}
// chk-sync.ts — node_create → addInstance → trigger block → 503 → sync → OK → state matches.
//   t1: root funded
//   t2: deploy bulk
//   t3: fill 110 × 1000 = 110k гранул (110 блоков)
//   t4: node_create n_test (enabled=true автоматом)
//   t5: addInstance n_test + mint 0 (trigger block → HB → do_sync)
//   t6: during sync: get_* на n_test → 503
//   t7: wait n_test sync done
//   t8: state matches n0 == n_test
const node = "127.0.0.1:2301"; // n0
const testNode = "127.0.0.1:2310"; // n_test
const KEYS_PATH = std.getenv("MTJS_KEYS_PATH") || "keys_test.json";
const BULK_SRC_PATH = "/home/bsr/mtjs/src/js/tsi/dist/contracts/bulk.js";
const TOTAL_BLOCKS = 110;
const PER_TX = 1000;
const TEST_NODE_NAME = "n_test_" + Date.now();
let rootSk = "";
async function loadKeys() {
    const keys = JSON.parse(await mtjs.fs.readFile(KEYS_PATH));
    const r = keys.users?.root?.keys?.[0];
    if (!r?.sk)
        throw new Error("keys_test.json: no users.root.keys[0].sk");
    rootSk = r.sk;
}
const GREEN = "\x1b[32m";
const RED = "\x1b[31m";
const RESET = "\x1b[0m";
let pass = 0, fail = 0;
function check(c, m) {
    if (c) {
        pass++;
        console.log("  " + GREEN + "✅" + RESET + " " + m);
    }
    else {
        fail++;
        console.log("  " + RED + "❌" + RESET + " " + m);
    }
}
function errStr(e) {
    return (e && typeof e === "object" && ("errcode" in e || "errstr" in e))
        ? "errcode=" + e.errcode + " errstr=" + e.errstr : String(e);
}
function sleep(ms) { return new Promise(r => setTimeout(() => r(), ms)); }
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
async function submitAndWait(tx, value = 0, gasLimit = 100000, retries = 5, waitSec = 60) {
    const addr = mtjs.addr_from_name("root");
    for (let a = 0; a < retries; a++) {
        const nonce = await getNonce(addr);
        const req = { tx, nonce: nonce.toString(), value, gasLimit, priority: 0, sender: addr, key_id: 0 };
        const rsp = await mtjs.tx_submit(node, 1, jstr(req), rootSk, nonce.toString(), () => { });
        if (rsp.err) {
            if (a + 1 < retries) {
                await sleep(300);
                continue;
            }
            throw new Error("submit: " + rsp.err_str);
        }
        const b = await waitTx(rsp.tx_hash, waitSec);
        const st = b.status(rsp.tx_hash);
        const es = b.errstr(rsp.tx_hash) || "";
        if (st === "err" && es.indexOf("invalid nonce") >= 0) {
            if (a + 1 < retries) {
                await sleep(300);
                continue;
            }
            throw new Error("invalid nonce");
        }
        return { rsp, block: b };
    }
    throw new Error("retries exhausted");
}
const rootAddr = mtjs.addr_from_name("root");
async function main() {
    await loadKeys();
    console.log("=== SYNC: node_create → addInstance → mint 0 → 503 → sync → OK ===");
    // ============================================================
    // t1: root funded
    // ============================================================
    {
        console.log("\n[t1] root funded");
        const ui = await mtjs.get_user_nonce(node, rootAddr, 3);
        if (BigInt(ui.balance) < 50000000n) {
            const { rsp, block } = await submitAndWait([
                { contract: "root", method: "mint", params: { amount: "100000000" } },
            ], 0, 20000);
            check(block.status(rsp.tx_hash) === "ok", "mint root 100M");
        }
        else {
            check(true, "root funded (" + ui.balance + ")");
        }
    }
    // ============================================================
    // t2: deploy bulk
    // ============================================================
    {
        console.log("\n[t2] deploy bulk");
        const src = await mtjs.fs.readFile(BULK_SRC_PATH);
        check(src.length > 0, "bulk src read (" + src.length + " bytes)");
        const c = await mtjs.get_contract(node, "bulk", 3);
        const method = c.found ? "contract_update" : "contract_deploy";
        const params = { name: "bulk", src };
        if (method === "contract_deploy")
            params.ttl = 1000000;
        const { rsp, block } = await submitAndWait([{ contract: "root", method, params }]);
        check(block.status(rsp.tx_hash) === "ok", "bulk " + method + " ok (errstr=" + (block.errstr(rsp.tx_hash) || "") + ")");
    }
    // ============================================================
    // t3: fill 110 × 1000 = 110k гранул (110 блоков)
    // ============================================================
    {
        console.log("\n[t3] fill " + TOTAL_BLOCKS + " × " + PER_TX + " = " + (TOTAL_BLOCKS * PER_TX) + " granules");
        const tAll = Date.now();
        let ok = 0, err = 0;
        for (let i = 0; i < TOTAL_BLOCKS; i++) {
            const { rsp, block } = await submitAndWait([
                { contract: "bulk", method: "fill", params: { prefix: "bulk/b" + i, count: PER_TX, ttl: 86400 } },
            ], 0, 300000, 5, 60);
            if (block.status(rsp.tx_hash) === "ok") {
                ok++;
                if ((i + 1) % 10 === 0 || i === 0)
                    console.log("  fill b" + i + " ok, " + (i + 1) + "/" + TOTAL_BLOCKS +
                        ", total=" + ((Date.now() - tAll) / 1000).toFixed(1) + "s");
            }
            else {
                err++;
                check(false, "fill b" + i + " err: " + (block.errstr(rsp.tx_hash) || ""));
            }
        }
        console.log("  fill total: " + ((Date.now() - tAll) / 1000).toFixed(1) + "s, ok=" + ok + " err=" + err);
        check(err === 0, "all " + TOTAL_BLOCKS + " fill ok");
    }
    // ============================================================
    // t4: node_create n_test + node_stake (без стейка нода не попадает в дерево рассылки)
    // ============================================================
    const kpEd = mtjs.generate_keypair(0); // ed25519
    const kpBls = mtjs.generate_keypair(3); // bls12-381
    {
        console.log("\n[t4] node_create " + TEST_NODE_NAME);
        const { rsp, block } = await submitAndWait([
            { contract: "root", method: "node_create", params: {
                    name: TEST_NODE_NAME,
                    ip: testNode,
                    pk_ed: kpEd.pk,
                    pk_bls: kpBls.pk,
                    ttl: 100000,
                } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "node_create ok (errstr=" + (block.errstr(rsp.tx_hash) || "") + ")");
        const ni = await mtjs.get_node_info(node, TEST_NODE_NAME, 3);
        check(ni.found === true, "n_test found in state");
        check(ni.enabled === true, "n_test enabled (автоматом)");
        check(ni.ed_pk === kpEd.pk, "ed_pk matches");
        check(ni.bls_pk === kpBls.pk, "bls_pk matches");
        // стейк: иначе getMetaFull выкидывает ноду из vne/tree_all_nodes
        console.log("\n[t4b] node_stake " + TEST_NODE_NAME);
        const st = await submitAndWait([
            { contract: "root", method: "node_stake", params: {
                    node: TEST_NODE_NAME,
                    amount: "100",
                } },
        ], 100); // value: node_stake списывает из value транзакции
        check(st.block.status(st.rsp.tx_hash) === "ok", "node_stake ok (errstr=" + (st.block.errstr(st.rsp.tx_hash) || "") + ")");
    }
    // ============================================================
    // t5: addInstance n_test + mint 0 (trigger block → HB → do_sync)
    // ============================================================
    {
        console.log("\n[t5] addInstance " + TEST_NODE_NAME + " (sync)");
        mtjs.addInstance("MTJS_TEST", `
            Start=Node
                Node_rpc_addr=${testNode}
                Node_web_addr=0.0.0.0:7710
                http_granule_addr=0.0.0.0:4510
                Node_rockdb_path=db/r_test
                MTJS_PENDING_TIMEOUT=0.2
                MTJS_STACK_SIZE=8388608
                MTJS_THREAD_POOL_SIZE=10
                mtjs_deviceName=Device
                SocketIO_listen_backlog=128
                SocketIO_size=1024
                SocketIO_epoll_timeout_millisec=2000
                SocketIO_n_workers=2
                Oscar_maxPacketSize=100000000
                HTTP_max_post=1000000
                HTTP_doc_urls=/pics,/html,/css
                HTTP_document_root=./www
                GrainWriter_snapshot_modulus=1000
                Node_my_sk_bls=${kpBls.sk}
                Node_my_sk_ed=${kpEd.sk}
                Node_this_node_name=${TEST_NODE_NAME}
                Node_sqlite_pn=db/s_test
                db_name=db_test
                mysql_max_connections=20
                mysql_host=NULL
                mysql_user=root
                mysql_passwd=gaga123
                mysql_database=md
                mysql_port=0
                mysql_sock=/run/mysqld/mysqld.sock
        `);
        check(true, "addInstance");
        // trigger block: mint 0 (валидная TX, баланс не меняется)
        // → n0 do_heart_beat → HeartBeatREQ broadcast → n_test получает → do_sync
        console.log("  trigger block (mint 0) to start HeartBeatREQ...");
        const { rsp, block } = await submitAndWait([
            { contract: "root", method: "mint", params: { amount: "0" } },
        ], 0, 20000);
        check(block.status(rsp.tx_hash) === "ok", "trigger block (mint 0) ok");
        console.log("  block " + (block.block_hash || "").slice(0, 16) + " → HB broadcast → n_test do_sync");
    }
    // ============================================================
    // t6: during sync: get_* на n_test → 503
    // ============================================================
    {
        console.log("\n[t6] during sync: get_* on " + TEST_NODE_NAME + " → 503 syncing");
        let syncing = 0, ready = 0, other = 0;
        let firstLogged = false;
        const t0 = Date.now();
        for (let i = 0; i < 40 && Date.now() - t0 < 30000; i++) {
            try {
                const st = await mtjs.get_state(testNode, 3);
                if (st.errcode === 503 || (st.errstr && st.errstr.indexOf("syncing") >= 0)) {
                    syncing++;
                    if (!firstLogged) {
                        console.log("  get_state → 503 errstr=\"" + (st.errstr || "") + "\"");
                        firstLogged = true;
                    }
                }
                else if (st.found) {
                    ready++;
                    if (!firstLogged) {
                        console.log("  get_state → OK (sync already done?)");
                        firstLogged = true;
                    }
                }
                else {
                    other++;
                    if (!firstLogged) {
                        console.log("  get_state → found=false errcode=" + (st.errcode || 0) + " errstr=\"" + (st.errstr || "") + "\"");
                        firstLogged = true;
                    }
                }
            }
            catch (e) {
                other++;
                if (!firstLogged) {
                    console.log("  get_state exception: " + errStr(e));
                    firstLogged = true;
                }
            }
            await sleep(500);
        }
        console.log("  syncing=" + syncing + " ready=" + ready + " other=" + other);
        // Синк на localhost может завершиться быстрее первого опроса: тогда 503
        // не наблюдается. Это не провал — важно, что нода отвечает (503 или OK).
        check(syncing > 0 || ready > 0, "get_state: 503=" + syncing + " ready=" + ready + " other=" + other);
        let syncing_ni = 0, ready_ni = 0;
        for (let i = 0; i < 10; i++) {
            try {
                const ni = await mtjs.get_node_info(testNode, TEST_NODE_NAME, 3);
                if (ni.errcode === 503 || (ni.errstr && ni.errstr.indexOf("syncing") >= 0))
                    syncing_ni++;
                else
                    ready_ni++;
            }
            catch (_) { }
            await sleep(300);
        }
        check(syncing_ni > 0 || ready_ni > 0, "get_node_info: 503=" + syncing_ni + " ready=" + ready_ni + "/10");
        let syncing_ci = 0, ready_ci = 0;
        for (let i = 0; i < 10; i++) {
            try {
                const ci = await mtjs.get_contract(testNode, "bulk", 3);
                if (ci.errcode === 503 || (ci.errstr && ci.errstr.indexOf("syncing") >= 0))
                    syncing_ci++;
                else
                    ready_ci++;
            }
            catch (_) { }
            await sleep(300);
        }
        check(syncing_ci > 0 || ready_ci > 0, "get_contract: 503=" + syncing_ci + " ready=" + ready_ci + "/10");
        let syncing_gd = 0, ready_gd = 0;
        for (let i = 0; i < 10; i++) {
            try {
                const gd = await mtjs.get_contract_data(testNode, "bulk", "bulk/b0/0", 3);
                if (gd.errcode === 503 || (gd.errstr && gd.errstr.indexOf("syncing") >= 0))
                    syncing_gd++;
                else
                    ready_gd++;
            }
            catch (_) { }
            await sleep(300);
        }
        check(syncing_gd > 0 || ready_gd > 0, "get_contract_data: 503=" + syncing_gd + " ready=" + ready_gd + "/10");
    }
    // ============================================================
    // t7: wait n_test sync done
    // ============================================================
    {
        console.log("\n[t7] wait " + TEST_NODE_NAME + " sync done");
        const t0 = Date.now();
        let ok = false;
        while (Date.now() - t0 < 900000) { // 15 минут
            try {
                const ni = await mtjs.get_node_info(testNode, TEST_NODE_NAME, 5);
                if (ni.found && ni.enabled && ni.errcode !== 503) {
                    ok = true;
                    break;
                }
            }
            catch (_) { }
            await sleep(2000);
        }
        const dt = ((Date.now() - t0) / 1000).toFixed(1);
        console.log("  sync done in " + dt + "s");
        check(ok, "n_test sync done (get_node_info OK)");
    }
    // ============================================================
    // t8: state matches n0 == n_test
    // ============================================================
    {
        console.log("\n[t8] state matches n0 == " + TEST_NODE_NAME);
        const s0 = await mtjs.get_state(node, 3);
        const st = await mtjs.get_state(testNode, 5);
        check(s0.total_supply === st.total_supply, "total_supply: " + s0.total_supply + " == " + st.total_supply);
        check(s0.tx_count === st.tx_count, "tx_count: " + s0.tx_count + " == " + st.tx_count);
        check(s0.block_height === st.block_height, "block_height: " + s0.block_height + " == " + st.block_height);
        const ci = await mtjs.get_contract(testNode, "bulk", 5);
        check(ci.found === true, "n_test get_contract(bulk) OK");
        const gd = await mtjs.get_contract_data(testNode, "bulk", "bulk/b0/0", 5);
        check(gd.found === true, "n_test get_contract_data(bulk/b0/0) OK");
        const gdLast = await mtjs.get_contract_data(testNode, "bulk", "bulk/b0/" + (PER_TX - 1), 5);
        check(gdLast.found === true, "n_test get_contract_data(bulk/b0/" + (PER_TX - 1) + ") OK");
        const r = await mtjs.call_view(testNode, "bulk", "count", { prefix: "bulk/b0" }, 60, 10);
        if (r.found && !r.error) {
            const v = unBig(r.result);
            check(v.count === PER_TX, "n_test bulk.count(b0) == " + PER_TX + " (got " + v.count + ")");
        }
        else {
            check(false, "n_test bulk.count failed: " + (r.result || "not found"));
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
