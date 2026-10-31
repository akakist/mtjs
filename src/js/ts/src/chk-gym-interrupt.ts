#!/usr/local/bin/mtjs
import * as std from "std";

// chk-gym-interrupt.ts — interrupt_handler на живой ноде:
//   contract_deploy interrupt (src из interrupt.js)
//   loopN(1000)  → ok (мало, не прерывается)
//   loopN(1e9)   → err (interrupted: gasUsed > gasLimit) — rollback, газ списан
//   loop()       → err (while(true) → interrupted)
// node :2301 (n1). root — sender (hex).

const node = "127.0.0.1:2301";
const INTERRUPT_SRC_PATH = "/home/bsr/mtjs/src/js/tsi/dist/contracts/interrupt.js";
const GAS_LIMIT = 100000;

const rootSk = std.getenv("u_root_ed_sk") || std.getenv("k_root_ed_sk");
const rootPk = std.getenv("k_root_ed_pk");
if (!rootSk || !rootPk) throw new Error("no root sk/pk");

const GREEN = "\x1b[32m"; const RED = "\x1b[31m"; const RESET = "\x1b[0m";
let pass = 0; let fail = 0;
function check(cond: boolean, msg: string) {
    if (cond) { pass++; console.log("  " + GREEN + "✅" + RESET + " " + msg); }
    else      { fail++; console.log("  " + RED   + "❌" + RESET + " " + msg); }
}
function errStr(e: any): string {
    if (e && typeof e === "object" && ("errcode" in e || "errstr" in e))
        return "errcode=" + e.errcode + " errstr=" + e.errstr;
    return String(e);
}

const pending: { [h: string]: { resolve: (b: Block) => void; reject: (e: any) => void; timer: any } } = {};
mtjs.tx_subscribe(node, (b: Block) => {
    for (const h of b.tx_hashes) {
        const p = pending[h];
        if (p) { delete pending[h]; clearTimeout(p.timer); p.resolve(b); }
    }
});
function waitTx(h: string, to: number): Promise<Block> {
    return new Promise<Block>((res, rej) => {
        const timer = setTimeout(() => {
            delete pending[h];
            rej({ errcode: 1, errstr: "timed out (no block with tx " + h.slice(0,8) + ")" });
        }, to * 1000);
        pending[h] = { resolve: res, reject: rej, timer };
    });
}
async function getNonce(addr: string): Promise<bigint> {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    return BigInt(ui.nonce);
}
async function getBalance(addr: string): Promise<bigint> {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    return BigInt(ui.balance);
}
async function getNonceBal(addr: string): Promise<{ nonce: bigint; bal: bigint }> {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    return { nonce: BigInt(ui.nonce), bal: BigInt(ui.balance) };
}
async function waitNonceGt(addr: string, lastNonce: bigint, timeoutSec = 10): Promise<bigint> {
    const t0 = Date.now();
    while (Date.now() - t0 < timeoutSec * 1000) {
        const n = await getNonce(addr);
        if (n > lastNonce) return n;
        await new Promise<void>((r) => setTimeout(() => r(), 200));
    }
    throw new Error("waitNonceGt timeout: lastNonce=" + lastNonce.toString());
}
async function submitAndWait(sk: string, addr: string, tx: any[], value = 0, gasLimit = GAS_LIMIT) {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    const req = { tx, nonce: ui.nonce, value, gasLimit, gasPrice: 10 };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, ui.nonce, () => {});
    if (rsp.err) throw new Error("submit: " + rsp.err_str);
    const b = await waitTx(rsp.tx_hash, 10);
    await waitNonceGt(addr, BigInt(ui.nonce), 10);
    return { rsp, block: b };
}
async function submitAndWaitErr(sk: string, addr: string, tx: any[], value = 0, gasLimit = GAS_LIMIT) {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    const req = { tx, nonce: ui.nonce, value, gasLimit, gasPrice: 10 };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, ui.nonce, () => {});
    if (rsp.err) throw new Error("submit: " + rsp.err_str);
    const b = await waitTx(rsp.tx_hash, 10);
    await waitNonceGt(addr, BigInt(ui.nonce), 10);
    return { rsp, block: b };
}

const rootAddr = mtjs.addr_from_pk(rootPk!);

async function ensureRootFunded(minBalance: bigint) {
    const bal = await getBalance(rootAddr);
    console.log("  root balance:", bal.toString(), "min:", minBalance.toString());
    if (bal >= minBalance) { check(true, "root funded >= " + minBalance.toString()); return; }
    if (bal < 200000n) { check(false, "root balance too low for mint — clean db"); return; }
    const { rsp, block } = await submitAndWait(rootSk!, rootAddr, [
        { contract: "root", method: "mint", params: { amount: "100000000" } },
    ], 0, 20000);
    check(block.status(rsp.tx_hash) === "ok", "mint root 100M ok");
}
async function ensureInterrupt(src: string) {
    const c = await mtjs.get_contract(node, "interrupt", 3);
    if (c.found) {
        const { rsp, block } = await submitAndWait(rootSk!, rootAddr, [
            { contract: "root", method: "contract_update", params: { name: "interrupt", src } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "contract_update interrupt ok");
    } else {
        const { rsp, block } = await submitAndWait(rootSk!, rootAddr, [
            { contract: "root", method: "contract_deploy", params: { name: "interrupt", src } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "contract_deploy interrupt ok");
    }
}

async function main() {
    console.log("=== INTERRUPT (node " + node + ") ===");

    await ensureRootFunded(50_000_000n);

    let src = "";
    try { src = await mtjs.fs.readFile(INTERRUPT_SRC_PATH); }
    catch (e: any) { check(false, "read interrupt src: " + errStr(e)); return; }
    check(src.length > 0, "interrupt src read (" + src.length + " bytes)");

    await ensureInterrupt(src);

    // 1) loopN(1000) — ok (мало, не прерывается)
    {
        console.log("\n[1] loopN(1000) → ok");
        const { rsp, block } = await submitAndWait(rootSk!, rootAddr, [
            { contract: "interrupt", method: "loopN", params: { n: 1000 } },
        ]);
        check(block.status(rsp.tx_hash) === "ok", "loopN(1000) ok (errstr=" + block.errstr(rsp.tx_hash) + ")");
    }

    // 2) loopN(1000000000) — err (interrupted: gasUsed > gasLimit)
    {
        console.log("\n[2] loopN(1000000000) → err (interrupted)");
        const { nonce: n0, bal: b0 } = await getNonceBal(rootAddr);
        const { rsp, block } = await submitAndWaitErr(rootSk!, rootAddr, [
            { contract: "interrupt", method: "loopN", params: { n: 1000000000 } },
        ], 0, GAS_LIMIT);
        check(block.status(rsp.tx_hash) === "err", "status err");
        const es = block.errstr(rsp.tx_hash) || "";
        console.log("  errstr:", es);
        check(es.indexOf("interrupt") >= 0 || es.indexOf("gas") >= 0, "errstr mentions interrupt/gas");
        const { nonce: n1, bal: b1 } = await getNonceBal(rootAddr);
        check(n1 === n0 + 1n, "nonce +1");
        check(b1 <= b0, "balance not increased (gas charged): " + b0.toString() + " -> " + b1.toString());
    }

    // 3) loop() — err (while(true) → interrupted)
    {
        console.log("\n[3] loop() → err (while(true) → interrupted)");
        const { nonce: n0, bal: b0 } = await getNonceBal(rootAddr);
        const { rsp, block } = await submitAndWaitErr(rootSk!, rootAddr, [
            { contract: "interrupt", method: "loop", params: {} },
        ], 0, GAS_LIMIT);
        check(block.status(rsp.tx_hash) === "err", "status err");
        const es = block.errstr(rsp.tx_hash) || "";
        console.log("  errstr:", es);
        check(es.indexOf("interrupt") >= 0 || es.indexOf("gas") >= 0, "errstr mentions interrupt/gas");
        const { nonce: n1, bal: b1 } = await getNonceBal(rootAddr);
        check(n1 === n0 + 1n, "nonce +1");
        check(b1 <= b0, "balance not increased (gas charged): " + b0.toString() + " -> " + b1.toString());
    }

    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0) console.log(GREEN + "DONE" + RESET);
    else            console.log(RED   + "FAIL" + RESET);
}

main().catch((e: any) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => {}, 1000000);
export {};
