#!/usr/local/bin/mtjs
import * as std from "std";

const node = "127.0.0.1:2301";
const rootSk = std.getenv("u_root_ed_sk") || std.getenv("k_root_ed_sk");
const rootPk = std.getenv("k_root_ed_pk");
if (!rootSk || !rootPk) throw new Error("no root sk/pk");

const GREEN = "\x1b[32m"; const RED = "\x1b[31m"; const RESET = "\x1b[0m";
let pass = 0, fail = 0;
function check(c: boolean, m: string) {
    if (c) { pass++; console.log("  " + GREEN + "✅" + RESET + " " + m); }
    else   { fail++; console.log("  " + RED   + "❌" + RESET + " " + m); }
}
function errStr(e: any): string {
    return (e && typeof e === "object" && ("errcode" in e || "errstr" in e))
        ? "errcode=" + e.errcode + " errstr=" + e.errstr : String(e);
}

const rootAddr = mtjs.addr_from_pk(rootPk!);

// генерация случайного hex-адреса (32 байта)
function randomAddrHex(): string {
    const s = "0123456789abcdef";
    let out = "";
    for (let i = 0; i < 64; i++) out += s[Math.floor(Math.random() * 16)];
    return out;
}

async function main() {
    console.log("=== GET_USER_NONCE (NoCreate, no spam) ===");

    const rb = BigInt((await mtjs.get_user_nonce(node, rootAddr, 3)).balance);
    if (rb < 50000000n) {
        if (rb < 200000n) { check(false, "root low — clean db"); return; }
        // mint root — TBD (submit) — или check false
        check(false, "root low — clean db / mint root (TBD)");
        return;
    }

    const s0 = await mtjs.get_state(node, 3);
    console.log("  state0:", JSON.stringify(s0));
    const acc0 = s0.account_count;

    // t1: get_user_nonce неизвестный адрес — balance 0, nonce 0 (не создан)
    {
        console.log("\n[t1] get_user_nonce неизвестный адрес — balance 0, nonce 0");
        const unknown = randomAddrHex();
        const ui = await mtjs.get_user_nonce(node, unknown, 3);
        console.log("  ui:", JSON.stringify(ui));
        check(ui.nonce === "0", "nonce == 0 (got " + ui.nonce + ")");
        check(ui.balance === "0", "balance == 0 (got " + ui.balance + ")");
    }
    // t2: account_count не растёт (не создан)
    {
        console.log("\n[t2] account_count не растёт (адрес не создан)");
        // ещё несколько неизвестных
        for (let i = 0; i < 5; i++) {
            await mtjs.get_user_nonce(node, randomAddrHex(), 3);
        }
        const s1 = await mtjs.get_state(node, 3);
        console.log("  account_count:", acc0, "->", s1.account_count);
        check(s1.account_count === acc0, "account_count unchanged: " + acc0 + " -> " + s1.account_count);
    }

    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0) console.log(GREEN + "DONE" + RESET);
    else            console.log(RED + "FAIL" + RESET);
}

main().catch((e: any) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => {}, 1000000);
export {};
