import * as std from "std";
let out = [];
for (let i = 0; i < 10; i++) {
    const node = `127.0.0.1:${2300 + i}`;
    try { const s = await mtjs.get_state(node, 2); out.push(`n${i}:${s.block_height}`); }
    catch (e) { out.push(`n${i}:ERR`); }
}
console.log(out.join(" "));
