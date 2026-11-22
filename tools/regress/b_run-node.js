#!/usr/local/bin/mtjs
import * as std from "std";
const idx = parseInt(std.getenv("MTJS_NODE_IDX") || "0", 10);
const name = `n${idx}`;
const KEYS_PATH = std.getenv("MTJS_KEYS_PATH") || "keys_test.json";
(async () => {
    const keysRaw = await mtjs.fs.readFile(KEYS_PATH);
    const keys = JSON.parse(keysRaw);
    const k = keys.nodes && keys.nodes[name];
    if (!k) { console.log("keys_test.json: no node " + name); process.exit(1); }
    mtjs.addInstance(`MTJS${idx}`, `
        Start=Node
            Node_rpc_addr=127.0.0.1:${2300 + idx}
            Node_web_addr=0.0.0.0:${7700 + idx}
            Node_http_granule_addr=0.0.0.0:${4500 + idx}
            Node_rockdb_path=db/r${idx}
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
            Node_sqlite_pn=db/s${idx}
            db_name=db${idx}
            mysql_max_connections=20
            mysql_host=NULL
            mysql_user=root
            mysql_passwd=gaga123
            mysql_database=md
            mysql_port=0
            mysql_sock=/run/mysqld/mysqld.sock
    `);
    console.log("[b_run-node] n" + idx + " on 127.0.0.1:" + (2300 + idx));
})();
setInterval(() => {}, 1000000);
