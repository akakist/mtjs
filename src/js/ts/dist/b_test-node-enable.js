#!/usr/local/bin/mtjs
import * as std from "std";
const node = "127.0.0.1:2301";
let sk = std.getenv('k_root_ed_sk');
let root_pk = std.getenv('k_root_ed_pk');
// let u0 = std.getenv('k_u0_ed_pk');
// let u1 = std.getenv('k_u1_ed_pk');
// let u2 = std.getenv('k_u2_ed_pk');
// let u3 = std.getenv('k_u3_ed_pk');
// let u4 = std.getenv('k_u4_ed_pk');
// let users=[u0!,u1!,u2!,u3!,u4!];
// let users: string[] = [];
// for (let i = 0; i < 20; i++) {
//     let pk = std.getenv(`k_u${i}_ed_pk`);
//     if (pk) users.push(pk);
// }
// let nodes: string[] = [];
// for (let i = 0; i < 20; i++) {
//     nodes.push(`n${i}`);
// }
async function exec() {
    // while(true)
    // {
    mtjs.tx_subscribe(node, (params) => {
        console.log("tx report from js:", JSON.stringify(params));
    });
    const ui = await mtjs.get_user_info(node, root_pk, 1.5);
    const nonce = ui.nonce;
    console.log(ui);
    let tx = [
        {
            contract: "root",
            method: "node_enable",
            params: { node: "n10" }
        }
    ];
    // const m=mtjs.tx_sign(tx, sk!);
    // console.log("signed tx:", m);
    const rsp = await mtjs.tx_submit(node, 1, JSON.stringify(tx), sk, nonce);
    console.log(rsp);
    // }
}
//console.log(std.getenv("PATH"));
try {
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
}
catch (e) {
    console.log(e);
}
