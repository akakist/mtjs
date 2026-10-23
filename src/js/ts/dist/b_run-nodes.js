#!/usr/local/bin/mtjs
// Поднимает 10 нод n0..n9, держит процесс.
// Тесты (chk-all.js) запускать отдельным процессом.
const NODES = 10;
console.log("[b_run-nodes] starting", NODES, "nodes...");
for (let i = 0; i < NODES; i++) {
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
console.log("[b_run-nodes] instances created, waiting 0.5s...");
mtjs.utils.sleep(0.5);
console.log("[b_run-nodes] ready. nodes: n0..n" + (NODES - 1) + " on 127.0.0.1:2300.." + (2300 + NODES - 1));
console.log("[b_run-nodes] run tests in another terminal: mtjs ts/dist/chk-all.js");
console.log("[b_run-nodes] ctrl+c to stop");
// держим процесс
setInterval(() => { }, 1000000);
export {};
