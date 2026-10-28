#!/usr/local/bin/mtjs
import * as std from "std";

// chk-mint.ts — mint root -> root, mint + transfer u0..u19, mint 0.
// node :2301.
//
// Семантика (не баг C++):
// - mint: sender.balance += amount, gas = getGas("mint") + размер db_dump
// - transfer: amount <= t.value (value из req), t.value -= amount,
//   газ + (value - t.value) списывается с sender
// - emit mint/transfer лежит в children(tx_hash)[seqId].emits,
//   root-level emits(tx_hash) — только "result"/"error"

const node = "127.0.0.1:2301";

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

// ============================================================
// pending tx → block
// ============================================================
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

async function getNonceBal(addr: string): Promise<{ nonce: bigint; bal: bigint }> {
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    return { nonce: BigInt(ui.nonce), bal: BigInt(ui.balance) };
}
async function waitNonceGt(addr: string, lastNonce: bigint, timeoutSec = 10): Promise<bigint> {
    const t0 = Date.now();
    while (Date.now() - t0 < timeoutSec * 1000) {
        const { nonce } = await getNonceBal(addr);
        if (nonce > lastNonce) return nonce;
        await new Promise<void>((r) => setTimeout(() => r(), 200));
    }
    throw new Error("waitNonceGt timeout: lastNonce=" + lastNonce.toString());
}

// mint от root: value=0 (mint не тратит value), gasLimit=20000
async function mintFromRoot(amount: string) {
    const rootAddr = mtjs.addr_from_pk(rootPk!);
    const { nonce } = await getNonceBal(rootAddr);
    const req = {
        tx: [{ contract: "root", method: "mint", params: { amount } }],
        nonce: nonce.toString(),
        value: 0,
        gasLimit: 20000,
        gasPrice: 10,
    };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), rootSk!, nonce.toString(), () => {});
    if (rsp.err) throw new Error("submit: " + rsp.err_str);
    const b = await waitTx(rsp.tx_hash, 10);
    await waitNonceGt(rootAddr, nonce, 10);
    return { rsp, block: b };
}

// transfer от root: value >= amount (иначе "amount>t.value"),
// value резервируется, t.value -= amount, остаток возвращается.
async function transferFromRoot(to: string, amount: string) {
    const rootAddr = mtjs.addr_from_pk(rootPk!);
    const { nonce } = await getNonceBal(rootAddr);
    const req = {
        tx: [{ contract: "root", method: "transfer", params: { to, amount } }],
        nonce: nonce.toString(),
        value: 100000,       // >= amount + запас; резерв gasLimit*gasPrice+value = 200000+100000 = 300000
        gasLimit: 20000,
        gasPrice: 10,
    };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), rootSk!, nonce.toString(), () => {});
    if (rsp.err) throw new Error("submit: " + rsp.err_str);
    const b = await waitTx(rsp.tx_hash, 10);
    await waitNonceGt(rootAddr, nonce, 10);
    return { rsp, block: b };
}

// emit из children(tx_hash)[seqId].emits (mint/transfer — emit_command)
function childEmits(b: Block, txHash: string): { [seq: string]: any[] } {
    const ch = b.children(txHash) as any;
    const out: { [seq: string]: any[] } = {};
    if (ch) for (const k of Object.keys(ch)) out[k] = ch[k].emits;
    return out;
}
function findChildEmit(b: Block, txHash: string, name: string): any | null {
    const em = childEmits(b, txHash);
    for (const k of Object.keys(em)) for (const e of em[k]) if (e[0] === name) return e[1];
    return null;
}

// ============================================================
// t1: mint root -> root
// ============================================================
async function t1_mint_root() {
    console.log("\n[t1] mint root -> root");
    const rootAddr = mtjs.addr_from_pk(rootPk!);
    const { bal: b0 } = await getNonceBal(rootAddr);
    console.log("  root balance before:", b0.toString());

    const amount = "5000000";
    const { rsp, block } = await mintFromRoot(amount);
    const st = block.status(rsp.tx_hash);
    const es = block.errstr(rsp.tx_hash);
    console.log("  status:", st, "errstr:", es);
    console.log("  emits:", JSON.stringify(block.emits(rsp.tx_hash)));
    console.log("  children:", JSON.stringify(block.children(rsp.tx_hash)));

    check(st === "ok", "status ok");
    // mint emit в children(tx_hash)[seqId].emits
    const mintEmit: any = findChildEmit(block, rsp.tx_hash, "mint");
    check(mintEmit !== null, "mint emit present (in children)");
    if (mintEmit) {
        check(mintEmit.to === rootAddr, "mint to == root");
        check(mintEmit.amount === amount, "mint amount == " + amount);
    }
    const { bal: b1 } = await getNonceBal(rootAddr);
    console.log("  root balance after:", b1.toString());
    check(b1 > b0, "balance grew: " + b0.toString() + " -> " + b1.toString());
    // газ: getGas("mint")=95 + размер db_dump (250) = 345, *10 = 3450. Порог с запасом.
    check(b1 >= b0 + 5000000n - 10000n, "balance grew by ~amount (>= amount - 10000)");
}

// ============================================================
// t2: mint root + transfer u0..u19
// ============================================================
async function t2_mint_all_users() {
    console.log("\n[t2] mint root + transfer u0..u19");
    const rootAddr = mtjs.addr_from_pk(rootPk!);

    // 1) mint root 100000 (пополнение)
    const amount = "100000";
    const { rsp, block } = await mintFromRoot(amount);
    const st = block.status(rsp.tx_hash);
    check(st === "ok", "mint root " + amount + " ok");
    if (st !== "ok") {
        console.log("  errstr:", block.errstr(rsp.tx_hash));
        return;
    }

    // 2) для каждого u0..u19: transfer 1000 (value=100000)
    for (let i = 0; i < 20; i++) {
        const uSk = std.getenv("k_u" + i + "_ed_sk");
        const uPk = std.getenv("k_u" + i + "_ed_pk");
        if (!uSk || !uPk) { console.log("  u" + i + ": no keys, skip"); continue; }
        const uAddr = mtjs.addr_from_pk(uPk);
        const { bal: ub0 } = await getNonceBal(uAddr);

        try {
            const { rsp: trsp, block: tb } = await transferFromRoot(uAddr, "1000");
            const tst = tb.status(trsp.tx_hash);
            if (tst !== "ok") {
                check(false, "u" + i + " transfer status=" + tst + " errstr=" + tb.errstr(trsp.tx_hash));
                continue;
            }
            // transfer emit в children
            const trEmit: any = findChildEmit(tb, trsp.tx_hash, "transfer");
            check(trEmit !== null, "u" + i + " transfer emit present");
            const { bal: ub1 } = await getNonceBal(uAddr);
            check(ub1 === ub0 + 1000n, "u" + i + " +1000: " + ub0.toString() + " -> " + ub1.toString());
        } catch (e: any) {
            check(false, "u" + i + " transfer: " + errStr(e));
        }
    }
}

// ============================================================
// t3: mint 0 (граница)
// ============================================================
async function t3_mint_zero() {
    console.log("\n[t3] mint 0 (граница)");
    const rootAddr = mtjs.addr_from_pk(rootPk!);
    const { bal: b0 } = await getNonceBal(rootAddr);
    const { rsp, block } = await mintFromRoot("0");
    const st = block.status(rsp.tx_hash);
    console.log("  status:", st, "errstr:", block.errstr(rsp.tx_hash));
    check(st === "ok", "status ok");
    const { bal: b1 } = await getNonceBal(rootAddr);
    // amount=0, u->balance += 0, газ списан → b1 <= b0
    check(b1 <= b0, "balance not increased (gas only): " + b0.toString() + " -> " + b1.toString());
}

// ============================================================
// main
// ============================================================
(async () => {
    console.log("=== MINT ALL TESTS ===");
    await t1_mint_root();
    await t2_mint_all_users();
    await t3_mint_zero();
    console.log("\n=== RESULT: " + pass + " passed, " + fail + " failed ===");
    if (fail === 0) console.log(GREEN + "DONE" + RESET);
    else            console.log(RED   + "FAIL" + RESET);
})().catch((e: any) => console.log(RED + "ERR: " + errStr(e) + RESET));
setInterval(() => {}, 1000000);
export {};
