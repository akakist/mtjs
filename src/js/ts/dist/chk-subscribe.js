#!/usr/local/bin/mtjs
import * as std from "std";
const node = "127.0.0.1:2301";
// Ключ root берём из того же keys_test.json, с которым поднимали genesis.
// Env-переменные (u_root_ed_sk/k_root_ed_pk) — только как fallback.
const KEYS_PATH = std.getenv("MTJS_KEYS_PATH") || "keys_test.json";
let sk = "";
let pk = "";
async function loadKeys() {
    sk = std.getenv("u_root_ed_sk") || std.getenv("k_root_ed_sk") || "";
    pk = std.getenv("k_root_ed_pk") || "";
    if (sk && pk)
        return;
    const keys = JSON.parse(await mtjs.fs.readFile(KEYS_PATH));
    const r = keys.users?.root?.keys?.[0];
    if (!r?.sk)
        throw new Error(KEYS_PATH + ": no users.root.keys[0].sk");
    sk = r.sk;
    pk = r.pk || "";
}
let got = 0;
mtjs.tx_subscribe(node, (b) => {
    got++;
    console.log("cb#" + got + " block_hash:", b.block_hash);
    console.log("  tx_hashes:", JSON.stringify(b.tx_hashes));
    console.log("  finalized:", b.finalized);
});
async function main() {
    await loadKeys();
    const addr = mtjs.addr_from_name("root");
    const ui = await mtjs.get_user_nonce(node, addr, 2);
    const nonce = BigInt(ui.nonce);
    const tx = [{ contract: "root", method: "mint", params: { amount: "1" } }];
    const req = { tx, nonce: nonce.toString(), value: 1000000, gasLimit: 100000, priority: 0, sender: addr, key_id: 0 };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, nonce.toString(), () => { });
    console.log("submit:", rsp.err, rsp.err_str);
    const t0 = Date.now();
    while (got === 0 && Date.now() - t0 < 10000) {
        await new Promise((r) => setTimeout(() => r(), 50));
    }
    console.log("got:", got);
}
main().then(() => { console.log("OK"); std.exit(0); }).catch((e) => { console.log("ERR:", JSON.stringify(e)); std.exit(1); });
setInterval(() => { }, 1000000);
