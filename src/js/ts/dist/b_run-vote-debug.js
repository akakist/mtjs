#!/usr/local/bin/mtjs
import * as std from "std";
const node = "127.0.0.1:2301";
const sk = std.getenv("u_root_ed_sk") || std.getenv("k_root_ed_sk");
const pk = std.getenv("k_root_ed_pk");
function check(c, m) { console.log(c ? "  ✅ " + m : "  ❌ " + m); }
function errStr(e) { return (e && typeof e === "object" && ("errcode" in e || "errstr" in e)) ? "errcode=" + e.errcode + " errstr=" + e.errstr : String(e); }
// ============================================================
// 1. Поднять 10 нод (реальная сеть: 2300..2309, 7700..7709)
// ============================================================
console.log("[vote-debug] starting 10 nodes...");
for (let i = 0; i < 10; i++) {
    mtjs.addInstance(`MTJS${i}`, `
        Start=Node
            Node_rpc_addr=127.0.0.1:${2300 + i}
            Node_web_addr=0.0.0.0:${7700 + i}
            Node_rockdb_path=db/r${i}
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
            Node_my_sk_bls_env_key=k_node${i}_bls_sk
            Node_my_sk_ed_env_key=k_node${i}_ed_sk
            Node_this_node_name=n${i}
            Node_sqlite_pn=db/s${i}
            db_name=db${i}
    `);
}
mtjs.utils.sleep(0.5);
console.log("[vote-debug] ready. nodes n0..n9");
// ============================================================
// 2. Подписка на блоки + waitTx (до tx_submit!)
// ============================================================
const pending = {};
let subCount = 0;
mtjs.tx_subscribe(node, (b) => {
    subCount++;
    if (subCount <= 5 || subCount % 10 === 0)
        console.log("[sub] #" + subCount + " ts=" + b.block_timestamp + " txs=" + b.tx_hashes.length);
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
async function getNonce() {
    const ui = await mtjs.get_user_nonce(node, mtjs.addr_from_pk(pk), 3);
    return BigInt(ui.nonce);
}
async function submitVote(voteNode, param, value, ttl) {
    const nonce = await getNonce();
    const tx = [{ contract: "root", method: "vote", params: { node: voteNode, param, value, ttl } }];
    const req = { tx, nonce: nonce.toString(), value: 1000000, gasLimit: 100000, gasPrice: 10 };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, nonce.toString(), () => { });
    if (rsp.err)
        throw new Error("submit: " + rsp.err_str);
    console.log("  [" + voteNode + "] submitted tx=" + rsp.tx_hash.slice(0, 8));
    const b = await waitTx(rsp.tx_hash, 10);
    console.log("  [" + voteNode + "] block ts=" + b.block_timestamp + " status=" + b.status(rsp.tx_hash));
    return { rsp, block: b };
}
// ============================================================
// 3. Тест vote TTL: expire → vote_failed
// ============================================================
(async () => {
    console.log("[vote-debug] vote TTL: expire → vote_failed");
    if (!sk || !pk) {
        console.log("no sk/pk");
        return;
    }
    const param = "vote_exp_" + String(Date.now());
    const value = "99";
    console.log("  param:", param);
    // первый vote (n5, ttl=1) → vote_counted
    const { rsp: rsp1, block: b1 } = await submitVote("n5", param, value, "1");
    console.log("  [n5] children:", JSON.stringify(b1.children(rsp1.tx_hash)));
    // ждём 3 сек — блоки идут только при TX, поэтому шлём mint-фон
    console.log("  waiting 3s + bg mint...");
    let bgRunning = true;
    (async () => {
        while (bgRunning) {
            await new Promise((r) => setTimeout(() => r(), 1000));
            if (!bgRunning)
                break;
            try {
                const nonce = await getNonce();
                const tx = [{ contract: "root", method: "mint", params: { amount: "1" } }];
                const req = { tx, nonce: nonce.toString(), value: 1000000, gasLimit: 100000, gasPrice: 10 };
                const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, nonce.toString(), () => { });
                if (!rsp.err)
                    console.log("  [bg] mint tx=" + rsp.tx_hash.slice(0, 8));
            }
            catch (e) {
                console.log("  [bg] err:", errStr(e));
            }
        }
    })();
    await new Promise((r) => setTimeout(() => r(), 3000));
    bgRunning = false;
    console.log("  bg stopped, subCount=" + subCount);
    // второй vote (n6, ttl=1) → vote_failed
    const { rsp: rsp2, block: b2 } = await submitVote("n6", param, value, "1");
    const ch = b2.children(rsp2.tx_hash);
    let failed = false;
    if (ch)
        for (const k of Object.keys(ch)) {
            const em = ch[k].emits;
            console.log("  child[" + k + "]:", JSON.stringify(em));
            for (const e of em)
                if (e[0] === "vote_failed")
                    failed = true;
        }
    check(b2.status(rsp2.tx_hash) === "ok", "status ok (tx success, vote failed)");
    check(failed, "vote_failed present");
    console.log("[vote-debug] DONE");
})().catch((e) => console.log("[vote-debug] ERR:", errStr(e)));
setInterval(() => { }, 1000000);
