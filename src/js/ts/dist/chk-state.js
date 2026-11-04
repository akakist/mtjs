#!/usr/local/bin/mtjs
import * as std from "std";
const node = "127.0.0.1:2301";
function check(c, m) {
    console.log((c ? "  ✅ " : "  ❌ ") + m);
}
(async () => {
    console.log("[chk-state]");
    try {
        const s = await mtjs.get_state(node, 3);
        console.log("  " + JSON.stringify(s));
        check(s.found === true, "found");
        check(typeof s.total_supply === "string", "total_supply string");
        check(typeof s.total_stake === "string", "total_stake string");
        check(typeof s.account_count === "number", "account_count number");
        check(typeof s.node_count === "number", "node_count number");
        check(typeof s.contract_count === "number", "contract_count number");
        check(typeof s.tx_count === "number", "tx_count number");
        check(typeof s.block_height === "number", "block_height number");
        check(typeof s.genesis_time === "number", "genesis_time number");
        check(typeof s.chain_id === "number", "chain_id number");
        // после init_root: node_count = 10 (n0..n9), chain_id = 1
        check(s.chain_id === 1, "chain_id == 1");
        check(s.genesis_time > 0, "genesis_time > 0");
        console.log("DONE");
        std.exit(0);
    }
    catch (e) {
        console.log("ERR:", e && e.errstr ? e.errstr : String(e));
    }
})().catch((e) => console.log("ERR:", String(e)));
setInterval(() => { }, 1000000);
