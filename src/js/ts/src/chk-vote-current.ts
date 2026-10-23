#!/usr/local/bin/mtjs
import * as std from "std";

const node = "127.0.0.1:2301";
const sk = std.getenv("u_root_ed_sk") || std.getenv("k_root_ed_sk");
const pk = std.getenv("k_root_ed_pk");
if (!sk || !pk) throw new Error("no sk/pk");

async function main() {
    const addr = mtjs.addr_from_pk(pk!);
    const ui = await mtjs.get_user_nonce(node, addr, 2);
    let nonce = BigInt(ui.nonce);

    const uid = String(Date.now());
    const param = "vote_test_" + uid;
    const value = "42";
    console.log("uid:", uid, "param:", param);

    {
        const tx = [{ contract: "root", method: "vote", params: { node: "n5", param, value } }];
        const req = { tx, nonce: nonce.toString(), value: 1000000, gasLimit: 100000, gasPrice: 10 };
        const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk!, nonce.toString(), () => {});
        nonce += 1n;
        console.log("[n5] submit:", rsp.err, rsp.err_str);
        if (rsp.err) return;
        const b = await mtjs.tx_wait_block(node, rsp.tx_hash, 10);
        console.log("[n5] status:", b.status(rsp.tx_hash));
        console.log("[n5] emits:", JSON.stringify(b.emits(rsp.tx_hash)));
    }

    {
        const tx = [
            { contract: "root", method: "vote", params: { node: "n6", param, value } },
            { contract: "root", method: "vote", params: { node: "n7", param, value } },
            { contract: "root", method: "vote", params: { node: "n8", param, value } },
            { contract: "root", method: "vote", params: { node: "n9", param, value } },
        ];
        const req = { tx, nonce: nonce.toString(), value: 1000000, gasLimit: 100000, gasPrice: 10 };
        const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk!, nonce.toString(), () => {});
        nonce += 1n;
        console.log("[n6..n9] submit:", rsp.err, rsp.err_str);
        if (rsp.err) return;
        const b = await mtjs.tx_wait_block(node, rsp.tx_hash, 10);
        console.log("[n6..n9] status:", b.status(rsp.tx_hash));
        const ch = b.children(rsp.tx_hash);
        if (ch) for (const k of Object.keys(ch)) {
            console.log("  child[" + k + "] emits:", JSON.stringify(ch[k]));
        }
    }
}
main().then(() => console.log("OK")).catch((e: any) => console.log("ERR:", JSON.stringify(e)));
setInterval(() => {}, 1000000);
