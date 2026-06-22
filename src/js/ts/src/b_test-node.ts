#!/usr/local/bin/mtjs

import * as std from "std";
import { sleep } from "os";
const node="127.0.0.1:2301";
let sk=std.getenv('k_root_ed_sk');
let root_pk=std.getenv('k_root_ed_pk');
// let u0 = std.getenv('k_u0_ed_pk');
// let u1 = std.getenv('k_u1_ed_pk');
// let u2 = std.getenv('k_u2_ed_pk');
// let u3 = std.getenv('k_u3_ed_pk');
// let u4 = std.getenv('k_u4_ed_pk');
// let users=[u0!,u1!,u2!,u3!,u4!];
let users: string[] = [];
for (let i = 0; i < 20; i++) {
    let pk = std.getenv(`k_u${i}_ed_pk`);
    if (pk) users.push(mtjs.addr_from_pk(pk));
}
let nodes: string[] = [];
for (let i = 0; i < 20; i++) {
    nodes.push(`n${i}`);
}
async function exec() {
        while(true)
        {
            mtjs.tx_subscribe(node, (params)=>{
                console.log("tx report from js:", JSON.stringify(params));
            });
            const ui=await mtjs.get_user_info(node,mtjs.addr_from_pk(root_pk!),1.5)
            const nonce=ui.nonce;
            console.log(ui);
            let tx=
                [
                    {
                        contract:"root",
                        method:"mint",
                        params: { amount:"100000"}
                    }
                ];

            for (let i = 0; i < users.length; i++)
            {
                tx.push({
                    contract:"root",
                    method:"transfer",
                    params:{to:users[i], amount: `${100+i}`}
                } as any);
            }
            for(let i=0;i<nodes.length;i++)
            {
                tx.push({
                    contract:"root",
                    method:"node_stake",
                    params:{node:nodes[i], amount:`${i+10}`!}
                } as any);
            }
            for(let i=1;i<nodes.length;i++)
            {
                tx.push({
                    contract:"root",
                    method:"node_unstake",
                    params:{node:nodes[i], amount:`${i+5}`!}
                } as any);
            }

            // const m=mtjs.tx_sign(tx, sk!);
            // console.log("signed tx:", m);

            const rsp=await mtjs.tx_submit(node,1, JSON.stringify(tx), sk!, nonce);
            console.log(rsp);
            sleep(1000);
        }
}

console.log(std.getenv("PATH"));
try{
    const nums=[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19];
    for(let i of nums)
    {
        mtjs.addInstance(`MTJS${i}`,`
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
                Node_my_sk_bls_env_key=k_node${i}_bls_sk
                Node_my_sk_ed_env_key=k_node${i}_ed_sk
                Node_this_node_name=n${i}
                Node_sqlite_pn=db/s${i}
        `);
    }
     sleep(200);
    console.log("Start");
    try{
        const sk=std.getenv('u_root_ed_sk');
        setInterval(()=>{

        },5000);
        await exec();
    }
    catch(e)
    {
        console.log("catched: ",e);
    }

} catch(e){console.log(e);}