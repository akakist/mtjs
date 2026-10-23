#!/usr/local/bin/mtjs
import * as std from "std";

const node = "127.0.0.1:2301";
const sk = std.getenv("u_root_ed_sk") || std.getenv("k_root_ed_sk");
const pk = std.getenv("k_root_ed_pk");
if (!sk || !pk) throw new Error("no sk/pk");

function check(cond: boolean, msg: string) {
    if (cond) { console.log("  ✅", msg); }
    else      { console.log("  ❌", msg); }
}
function errStr(e: any): string {
    if (e && typeof e === "object" && ("errcode" in e || "errstr" in e))
        return "errcode=" + e.errcode + " errstr=" + e.errstr;
    return String(e);
}

async function main() {
    console.log("[t5] get_block (next block)");
    const addr = mtjs.addr_from_pk(pk!);
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    const nonce = BigInt(ui.nonce);

    // 1. подписаться ДО сборки блока
    const p = mtjs.get_block(node, 10);

    // 2. отправить TX, чтобы блок собрался
    const tx = [{ contract: "root", method: "mint", params: { amount: "1" } }];
    const req = { tx, nonce: nonce.toString(), value: 1000000, gasLimit: 100000, gasPrice: 10 };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk!, nonce.toString(), () => {});
    console.log("  submit:", rsp.err, rsp.err_str);
    if (rsp.err) { check(false, "submit failed"); return; }

    // 3. дождаться блока
    const t0 = Date.now();
    const b = await p;
    const dt = Date.now() - t0;
    console.log("  block_hash:", b.block_hash, "tx_hashes:", b.tx_hashes.length, "in", dt, "ms");
    console.log("  includes my tx:", b.tx_hashes.indexOf(rsp.tx_hash) >= 0);
    check(b.block_hash.length > 0, "block_hash non-empty");
    check(Array.isArray(b.tx_hashes), "tx_hashes is array");
    check(dt < 10500, "resolved < 10.5s");
}

main()
    .then(() => console.log("OK"))
    .catch((e: any) => console.log("ERR:", errStr(e)));

setInterval(() => {}, 1000000);