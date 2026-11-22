#!/usr/local/bin/mtjs
import * as std from "std";
// JSON.stringify с поддержкой BigInt (значения гранул — 128-битные)
function jstr(v) {
    return JSON.stringify(v, (_k, x) => (typeof x === "bigint" ? Number(x) : x));
}
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
async function main() {
    await loadKeys();
    const addr = mtjs.addr_from_name("root");
    const ui = await mtjs.get_user_nonce(node, addr, 2);
    const bad = (BigInt(ui.nonce) + 100000n).toString();
    console.log("bad nonce:", bad, "(real:", ui.nonce, ")");
    const tx = [{ contract: "root", method: "mint", params: { amount: "1" } }];
    const req = { tx, nonce: bad, value: 1000000, gasLimit: 100000, priority: 0, sender: addr, key_id: 0 };
    const rsp = await mtjs.tx_submit(node, 1, jstr(req), sk, bad, () => { });
    console.log("submit:", rsp.err, rsp.err_str, rsp.tx_hash);
    if (rsp.err) {
        console.log("rejected before block");
        return;
    }
    const block = await mtjs.tx_wait_block(node, rsp.tx_hash, 10);
    console.log("status:", block.status(rsp.tx_hash));
    console.log("errstr:", block.errstr(rsp.tx_hash));
    console.log("emits:", jstr(block.emits(rsp.tx_hash)));
}
main().then(() => { console.log("OK"); std.exit(0); }).catch((e) => { console.log("ERR:", jstr(e)); std.exit(1); });
