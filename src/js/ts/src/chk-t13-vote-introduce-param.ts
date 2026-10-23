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

async function submitAndWait(tx: any[]) {
    const nonce = await getNonce();
    const req = { tx, nonce: nonce.toString(), value: 1000000, gasLimit: 100000, gasPrice: 10 };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk!, nonce.toString(), () => {});
    if (rsp.err) throw new Error("submit: " + rsp.err_str);
    console.log("  submitted tx=" + rsp.tx_hash.slice(0,8));
    const b = await waitTx(rsp.tx_hash, 10);
    console.log("  block ts=" + b.block_timestamp + " status=" + b.status(rsp.tx_hash));
    return { rsp, block: b };
}

(async () => {
    console.log("[t13] vote: introduce validator_min_stake");
    if (!sk || !pk) { console.log("no sk/pk"); return; }

    const v0 = await mtjs.get_values(node, 3);
    console.log("  before: validator_min_stake =", v0.fees["validator_min_stake"]);
    console.log("  before: vote =", v0.fees["vote"]);

    const param = "validator_min_stake";
    const value = "100";
    const tx = [
        { contract: "root", method: "vote", params: { node: "n5", param, value, ttl: "60" } },
        { contract: "root", method: "vote", params: { node: "n6", param, value, ttl: "60" } },
        { contract: "root", method: "vote", params: { node: "n7", param, value, ttl: "60" } },
        { contract: "root", method: "vote", params: { node: "n8", param, value, ttl: "60" } },
        { contract: "root", method: "vote", params: { node: "n9", param, value, ttl: "60" } },
    ];
    const { rsp, block: b } = await submitAndWait(tx);
    const ch = b.children(rsp.tx_hash);
    let applied = false;
    if (ch) for (const k of Object.keys(ch)) {
        const em = (ch as any)[k].emits as any[];
        for (const e of em) if (e[0] === "vote_applied") applied = true;
    }
    check(b.status(rsp.tx_hash) === "ok", "status ok");
    check(applied, "vote_applied present");

    const v1 = await mtjs.get_values(node, 3);
    console.log("  after: validator_min_stake =", v1.fees["validator_min_stake"]);
    console.log("  after: vote =", v1.fees["vote"]);
    check(v1.fees["validator_min_stake"] === "100", "validator_min_stake === '100'");
    console.log("DONE");
})().catch((e: any) => console.log("ERR:", errStr(e)));
setInterval(() => {}, 1000000);
export{}
