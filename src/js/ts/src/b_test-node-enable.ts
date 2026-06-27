#!/usr/local/bin/mtjs

import * as std from "std";
import { sleep } from "os";
const node="127.0.0.1:2301";
let sk=std.getenv('k_root_ed_sk');
let root_pk=std.getenv('k_root_ed_pk');
const start = performance.now();
async function exec() {
            mtjs.tx_subscribe(node, (params)=>{
                // console.log("tx report from js:", JSON.stringify(params));
            });
            const ui=await mtjs.get_user_nonce(node,mtjs.addr_from_pk(root_pk!),1.5)
            const nonce=ui.nonce;
            console.log(ui);
            let tx=
                [
                    {
                        contract:"root",
                        method:"node_enable",
                        params: { node:"n9" }
                    }
                ];
            const rsp=await mtjs.tx_submit(node,1, JSON.stringify(tx), sk!, nonce, (obj)=>{
                console.log(JSON.stringify(obj,null,2) );
                const elapsed = performance.now() - start;
                console.log("elapsed "+elapsed);
            });
            console.log(rsp);
}

console.log(std.getenv("PATH"));
try{
    console.log("Start");
    try{
        const sk=std.getenv('u_root_ed_sk');
        await exec();
    }
    catch(e)
    {
        console.log("catched: ",e);
    }

} catch(e){console.log(e);}