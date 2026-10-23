import * as std from "std";
import { sleep } from "os";

export const node = "127.0.0.1:2301";
export const sk = std.getenv("u_root_ed_sk") || std.getenv("k_root_ed_sk");
export const pk = std.getenv("k_root_ed_pk");

export let pass = 0;
export let fail = 0;

export function check(cond: boolean, msg: string) {
    if (cond) { pass++; console.log("  ✅", msg); }
    else      { fail++; console.log("  ❌", msg); }
}

export function errStr(e: any): string {
    if (e && typeof e === "object" && ("errcode" in e || "errstr" in e))
        return "errcode=" + e.errcode + " errstr=" + e.errstr;
    return String(e);
}

export function bootstrap() {
    console.log("[bootstrap] starting 10 nodes...");
    for (let i of [0,1,2,3,4,5,6,7,8,9]) {
        mtjs.addInstance(`MTJS${i}`, `
            Start=Node
                Node_rpc_addr=127.0.0.1:${2300 + i}
                Node_web_addr=0.0.0.0:${7700 + i}
                Node_rockdb_path=db/r${i}
                MTJS_PENDING_TIMEOUT=0.2
                MTJS_STACK_SIZE=8388608
                MTJS_THREAD_POOL_SIZE=10
                mtjs_deviceName=Device
                SocketIO_listen_backlog=128
                SocketIO_size=1024
                SocketIO_epoll_timeout_millisec=2000
                SocketIO_n_workers=2
                Oscar_maxPacketSize=33554432
                HTTP_max_post=1000000
                HTTP_doc_urls=/pics,/html,/css
                HTTP_document_root=./www
                GrainWriter_snapshot_modulus=1000
                Node_my_sk_bls_env_key=k_node${i}_bls_sk
                Node_my_sk_ed_env_key=k_node${i}_ed_sk
                Node_this_node_name=n${i}
                Node_sqlite_pn=db/s${i}
                db_name=db${i}
                mysql_max_connections=20
                 mysql_host=NULL
                 mysql_user=root
                 mysql_passwd=gaga123
                 mysql_database=md
                 mysql_port=0
                 mysql_sock=/run/mysqld/mysqld.sock
        `);
    }
    console.log("[bootstrap] waiting 300ms...");
    sleep(300);
    console.log("[bootstrap] done");
}

export async function getNonce(): Promise<bigint> {
    const addr = mtjs.addr_from_pk(pk!);
    const ui = await mtjs.get_user_nonce(node, addr, 3);
    return BigInt(ui.nonce);
}

export async function submitAndWait(userSk: string, nonce: bigint, tx: any[], timeoutSec = 10) {
    const req = { tx, nonce: nonce.toString(), value: 1000000, gasLimit: 100000, gasPrice: 10 };
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), userSk, nonce.toString(), () => {});
    if (rsp.err) throw new Error(`submit err: ${rsp.err_str}`);
    const block = await mtjs.tx_wait_block(node, rsp.tx_hash, timeoutSec);
    return { rsp, block };
}
