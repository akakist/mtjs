#!/usr/local/bin/mtjs
import * as std from "std";
import { sleep } from "os";
const node = "127.0.0.1:2301";
let sk = std.getenv('k_root_ed_sk');
let root_pk = std.getenv('k_root_ed_pk');
async function exec() {
        mtjs.tx_subscribe(node, (params) => {
            console.log("tx report from js:", JSON.stringify(params));
        });
    while (true) {
	console.log("gaga");
        const ui = await mtjs.get_user_info(node, root_pk, 1);
        const nonce = ui.nonce;
        console.log(ui);
        const rsp = await mtjs.mint(node, sk, { amount: "2000", timeout: 4, nonce: nonce });
        console.log(rsp);
	sleep(1100);
    }
}
console.log(std.getenv("PATH"));
try {
    const nums = [0, 1, 2, 3, 4];
    for (let i of nums) {
        // if(i==3) continue;
        mtjs.addInstance(`MTJS${i}`, `
            Start=Node
                Node_rpc_addr=127.0.0.1:${2300 + i}
                Node_web_addr=127.0.0.1:${7700 + i}
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
                Node_my_sk_bls_env_key=k_n${i}_bls_sk
                Node_my_sk_ed_env_key=k_n${i}_ed_sk
                Node_this_node_name=n${i}
                Node_sqlite_pn=db/s${i}

        `);
    }
    sleep(200);
    console.log("Start");
    try {
        const sk = std.getenv('u_root_ed_sk');
        setInterval(() => {
        }, 5000);
        await exec();
    }
    catch (e) {
        console.log("catched: ", e);
    }
    // mtjs.get_user_info("127.0.0.1:2345", { sk: sk , timeout: 1 })
    // .then(o=>{
    //     console.log(o)
    // })
    // .catch(e=> {
    //     console.log("Error:", e)
    // });
}
catch (e) {
    console.log(e);
}
