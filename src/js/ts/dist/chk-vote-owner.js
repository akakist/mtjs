#!/usr/local/bin/mtjs
import * as std from "std";
const node = "127.0.0.1:2301";
const u1_sk = std.getenv("k_u1_ed_sk");
const u1_pk = std.getenv("k_u1_ed_pk");
async function main() {
    if (!u1_sk || !u1_pk) {
        console.log("no k_u1_* keys");
        return;
    }
    const addr = mtjs.addr_from_pk(u1_pk);
    const ui = await mtjs.get_user_nonce(node, addr, 2);
    const nonce = BigInt(ui.nonce);
    console.log("u1 addr:", addr, "nonce:", nonce.toString());
    const param = "vote_owner_" + String(Date.now());
    const tx = [{ contract: "root", method: "vote", params: { node: "n5", param, value: "1" } }];
    const req = { tx, nonce: nonce.toString(), value: 1000000, gasLimit: 100000, gasPrice: 10 };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), u1_sk, nonce.toString(), () => { });
    console.log("submit:", rsp.err, rsp.err_str, rsp.tx_hash);
    if (rsp.err)
        return;
    const b = await mtjs.tx_wait_block(node, rsp.tx_hash, 10);
    console.log("status:", b.status(rsp.tx_hash));
    console.log("errstr:", b.errstr(rsp.tx_hash));
    console.log("emits:", JSON.stringify(b.emits(rsp.tx_hash)));
}
main().then(() => console.log("OK")).catch((e) => console.log("ERR:", JSON.stringify(e)));
setInterval(() => { }, 1000000);
