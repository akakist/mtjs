#!/usr/local/bin/mtjs
import * as std from "std";
const node = "127.0.0.1:2301";
const sk = std.getenv("u_root_ed_sk") || std.getenv("k_root_ed_sk");
const pk = std.getenv("k_root_ed_pk");
if (!sk || !pk)
    throw new Error("no sk/pk");
let got = 0;
mtjs.tx_subscribe(node, (b) => {
    got++;
    console.log("cb#" + got + " block_hash:", b.block_hash);
    console.log("  tx_hashes:", JSON.stringify(b.tx_hashes));
    console.log("  finalized:", b.finalized);
});
async function main() {
    const addr = mtjs.addr_from_pk(pk);
    const ui = await mtjs.get_user_nonce(node, addr, 2);
    const nonce = BigInt(ui.nonce);
    const tx = [{ contract: "root", method: "mint", params: { amount: "1" } }];
    const req = { tx, nonce: nonce.toString(), value: 1000000, gasLimit: 100000, gasPrice: 10 };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, nonce.toString(), () => { });
    console.log("submit:", rsp.err, rsp.err_str);
    const t0 = Date.now();
    while (got === 0 && Date.now() - t0 < 10000) {
        await new Promise((r) => setTimeout(() => r(), 50));
    }
    console.log("got:", got);
}
main().then(() => console.log("OK")).catch((e) => console.log("ERR:", JSON.stringify(e)));
setInterval(() => { }, 1000000);
