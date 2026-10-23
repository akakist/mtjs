#!/usr/local/bin/mtjs
import * as std from "std";

const node = "127.0.0.1:2301";
const sk = std.getenv("u_root_ed_sk") || std.getenv("k_root_ed_sk");
const pk = std.getenv("k_root_ed_pk");
function check(c: boolean, m: string) { console.log(c ? "  ✅ " + m : "  ❌ " + m); }
function errStr(e: any): string { return (e && typeof e==="object" && ("errcode" in e||"errstr" in e)) ? "errcode="+e.errcode+" errstr="+e.errstr : String(e); }

async function getNonce(): Promise<bigint> {
    const ui = await mtjs.get_user_nonce(node, mtjs.addr_from_pk(pk!), 3);
    return BigInt(ui.nonce);
}

const pending: { [txHash: string]: { resolve: (b: Block) => void; reject: (e: any) => void; timer: any } } = {};
let subCount = 0;
mtjs.tx_subscribe(node, (b: Block) => {
    subCount++;
    if (subCount <= 5 || subCount % 10 === 0)
        console.log("  [sub] #" + subCount + " ts=" + b.block_timestamp + " txs=" + b.tx_hashes.length);
    for (const h of b.tx_hashes) {
        const p = pending[h];
        if (p) { delete pending[h]; clearTimeout(p.timer); p.resolve(b); }
    }
});

function waitTx(txHash: string, timeoutSec: number): Promise<Block> {
    return new Promise<Block>((resolve, reject) => {
        const timer = setTimeout(() => {
            delete pending[txHash];
            reject({ errcode: 1, errstr: "timed out (no block with tx " + txHash.slice(0,8) + ")" });
        }, timeoutSec * 1000);
        pending[txHash] = { resolve, reject, timer };
    });
}

let bgRunning = true;
let bgCount = 0;
async function bgHeartbeat() {
    while (bgRunning) {
        await new Promise<void>((r) => setTimeout(() => r(), 1000));
        if (!bgRunning) break;
        try {
            const nonce = await getNonce();
            const tx = [{ contract: "root", method: "mint", params: { amount: "1" } }];
            const req = { tx, nonce: nonce.toString(), value: 1000000, gasLimit: 100000, gasPrice: 10 };
            const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk!, nonce.toString(), () => {});
            if (rsp.err) { console.log("  [bg] submit err:", rsp.err, rsp.err_str); continue; }
            bgCount++;
            console.log("  [bg] mint #" + bgCount + " tx=" + rsp.tx_hash.slice(0,8));
        } catch (e: any) { console.log("  [bg] err:", errStr(e)); }
    }
}

async function submitVoteAndWait(voteNode: string, param: string, value: string, ttl: string) {
    const nonce = await getNonce();
    const tx = [{ contract: "root", method: "vote", params: { node: voteNode, param, value, ttl } }];
    const req = { tx, nonce: nonce.toString(), value: 1000000, gasLimit: 100000, gasPrice: 10 };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk!, nonce.toString(), () => {});
    if (rsp.err) throw new Error("submit: " + rsp.err_str);
    console.log("  [" + voteNode + "] submitted tx=" + rsp.tx_hash.slice(0,8));
    const b = await waitTx(rsp.tx_hash, 10);
    console.log("  [" + voteNode + "] block ts=" + b.block_timestamp + " status=" + b.status(rsp.tx_hash));
    return { rsp, block: b };
}

(async () => {
    console.log("[t7e] vote TTL: expire → vote_failed");
    if (!sk || !pk) { console.log("no sk/pk"); return; }

    console.log("  starting bg heartbeat (mint fire-and-forget, 1s)...");
    bgHeartbeat();
    await new Promise<void>((r) => setTimeout(() => r(), 1500));
    console.log("  bg sent:", bgCount, "tx, subCount:", subCount);

    const param = "vote_exp_" + String(Date.now());
    const value = "99";
    console.log("  param:", param);

    const { rsp: rsp1, block: b1 } = await submitVoteAndWait("n5", param, value, "1");
    console.log("  [n5] children:", JSON.stringify(b1.children(rsp1.tx_hash)));

    console.log("  waiting 3s (bg keeps blocks flowing)...");
    await new Promise<void>((r) => setTimeout(() => r(), 3000));
    console.log("  bg sent total:", bgCount, "subCount:", subCount);

    const { rsp: rsp2, block: b2 } = await submitVoteAndWait("n6", param, value, "1");
    const ch = b2.children(rsp2.tx_hash);
    let failed = false;
    if (ch) for (const k of Object.keys(ch)) {
        const em = (ch as any)[k].emits as any[];
        console.log("  child[" + k + "]:", JSON.stringify(em));
        for (const e of em) if (e[0] === "vote_failed") failed = true;
    }
    check(b2.status(rsp2.tx_hash) === "ok", "status ok (tx success, vote failed)");
    check(failed, "vote_failed present");

    bgRunning = false;
    console.log("DONE");
})().catch((e: any) => { bgRunning = false; console.log("ERR:", errStr(e)); });
setInterval(() => {}, 1000000);
export{}
