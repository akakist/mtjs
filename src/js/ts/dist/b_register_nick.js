#!/usr/local/bin/mtjs
import * as std from "std";
import { sleep } from "os";
// import { RequestType } from "./types/mtjs";
const node = "127.0.0.1:2301";
let sk = std.getenv('u_root_ed_sk');
console.log("zaza1");
async function exec() {
    // while(true)
    // {
    //     mtjs.tx_subscribe(node, (params)=>{
    //         console.log("tx report from js:", JSON.stringify(params));
    //     });
    //     const ui=await mtjs.get_user_info(node, { sk: sk , timeout: 1 })
    //     const nonce=ui.nonce;
    //     console.log(ui);
    //     const rsp=await mtjs.mint(node, { sk:sk, amount:"2000", timeout:4, nonce: nonce })
    //     console.log(rsp);
    //     sleep(1000);
    // }
    while (true) {
        console.log("zaza3");
        mtjs.tx_subscribe(node, (params) => {
            console.log("tx report from js:", JSON.stringify(params));
        });
        console.log("zaza4");
        const ui = await mtjs.get_user_info(node, "root", 1);
        console.log("ui " + ui);
        // return;
        console.log("zaza5");
        // const res=await mtjs.tx_submit(node,"root", ui.nonce,sk!,1000,[{type: "register_user", nick:"Sergey"}]);
        // console.log("res" + res);
        console.log("zaza6");
        sleep(1000);
    }
}
console.log("zaza2");
try {
    exec().then((e) => console.log("yhen " + e)).catch((e) => { console.log("exc1 " + e); });
}
catch (e) {
    console.log("exc2 " + e);
}
sleep(1000000);
