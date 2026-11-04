#!/usr/local/bin/mtjs
import * as std from "std";
import { sleep } from "os";

const node = "127.0.0.1:2301";
const KEYS_PATH = std.getenv("MTJS_KEYS_PATH") || "keys_test.json";

(async () => {
    console.log(std.getenv("PATH"));

    let keysRaw = "";
    try { keysRaw = await mtjs.fs.readFile(KEYS_PATH); }
    catch (e: any) { console.log("readFile " + KEYS_PATH + " err:", String(e)); process.exit(1); }
    const keys = JSON.parse(keysRaw);

    const root = keys.users && keys.users.root && keys.users.root.keys && keys.users.root.keys[0];
    if (!root || !root.sk || !root.pk) { console.log("keys_test.json: no users.root.keys[0] sk/pk"); process.exit(1); }
    const sk = root.sk;

    const users: string[] = [];
    for (let i = 0; i < 10; i++) {
        users.push(mtjs.addr_from_name("u" + i));
    }
    const nodes: string[] = [];
    for (let i = 0; i < 10; i++) nodes.push(`n${i}`);

    const rootAddr = mtjs.addr_from_name("root");

    mtjs.tx_subscribe(node, (b: Block) => {
    });

    async function exec() {
        while (true) {
            const ui = await mtjs.get_user_nonce(node, rootAddr, 1.5);
            const nonce = ui.nonce;
            console.log(ui);

            let tx: any[] = [
                { contract: "root", method: "mint", params: { amount: "100000" } },
            ];
            for (let i = 0; i < users.length; i++) {
                tx.push({ contract: "root", method: "transfer",
                          params: { to: users[i], amount: `${100 + i}` } });
            }
            for (let i = 0; i < nodes.length; i++) {
                tx.push({ contract: "root", method: "node_stake",
                          params: { node: nodes[i], amount: `${i + 10}` } });
            }
            for (let i = 1; i < nodes.length; i++) {
                tx.push({ contract: "root", method: "node_unstake",
                          params: { node: nodes[i], amount: `${i + 5}` } });
            }

            const req = {
                tx: tx,
                nonce: nonce,
                value: 100000,
                gasLimit: 20000,
                priority: 0,
                sender: rootAddr,
                key_id: 0,
            };
            const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(req), sk, nonce, (obj) => {
                console.log("TX REPORT " + JSON.stringify(obj));
            });
            console.log(rsp);
            sleep(200);
        }
    }

    const nums = [0,1,2,3,4,5,6,7,8,9];
    for (let i of nums) {
        const name = `n${i}`;
        const k = keys.nodes && keys.nodes[name];
        if (!k) { console.log("keys_test.json: no node " + name); process.exit(1); }

        mtjs.addInstance(`MTJS${i}`, `
            Start=Node
                Node_rpc_addr=127.0.0.1:${2300+i}
                Node_web_addr=0.0.0.0:${7700+i}
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
                Node_my_sk_bls=${k.bls_sk}
                Node_my_sk_ed=${k.ed_sk}
                Node_this_node_name=${name}
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
    sleep(200);
    console.log("Start");
    try {
        setInterval(() => {}, 5000);
        await exec();
    } catch (e) {
        console.log("catched: ", e);
    }
})();

