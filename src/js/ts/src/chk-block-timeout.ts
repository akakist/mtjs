#!/usr/local/bin/mtjs
const fake = "00".repeat(32);
mtjs.tx_wait_block("127.0.0.1:2301", fake, 1)
    .then((b: Block) => console.log("UNEXPECTED:", b.block_hash))
    .catch((e: any) => console.log("rejected:", JSON.stringify(e)));
setInterval(() => {}, 1000000);
