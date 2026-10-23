#!/usr/local/bin/mtjs
import * as std from "std";
const node = "127.0.0.1:2301";
const sk = std.getenv("u_root_ed_sk") || std.getenv("k_root_ed_sk");
const pk = std.getenv("k_root_ed_pk");
if (!sk || !pk)
    throw new Error("no sk/pk");
async function main() {
    const addr = mtjs.addr_from_pk(pk);
    const ui = await mtjs.get_user_nonce(node, addr, 2);
    const bad = (BigInt(ui.nonce) + 100000n).toString();
    console.log("bad nonce:", bad, "(real:", ui.nonce, ")");
    const tx = [{ contract: "root", method: "mint", params: { amount: "1" } }];
    const req = { tx, nonce: bad, value: 1000000, gasLimit: 100000, gasPrice: 10 };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, bad, () => { });
    console.log("submit:", rsp.err, rsp.err_str, rsp.tx_hash);
    if (rsp.err) {
        console.log("rejected before block");
        return;
    }
    const block = await mtjs.tx_wait_block(node, rsp.tx_hash, 10);
    console.log("status:", block.status(rsp.tx_hash));
    console.log("errstr:", block.errstr(rsp.tx_hash));
    console.log("emits:", JSON.stringify(block.emits(rsp.tx_hash)));
}
main().then(() => console.log("OK")).catch((e) => console.log("ERR:", JSON.stringify(e)));
