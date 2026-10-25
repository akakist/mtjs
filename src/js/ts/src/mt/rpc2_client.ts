#!/usr/local/bin/mtjs
try{
    let cnt=0;
    const rpc=mtjs.rpc;
    const start = Math.floor(Date.now() / 1000);
    rpc.on_client("testMethod", (data:rpcClientResponse) => {
        rpc.sendTo("localhost:8099", "testMethod", { key: "hello" });
        cnt++;
        if(cnt%100000==0)
        {

            const t = Math.floor(Date.now() / 1000);
            console.log("Received on client:", JSON.stringify(data.params)," t ",t-start, " n ",cnt);
        }});

    for(let i=0; i<200; i++)    
        rpc.sendTo("localhost:8099", "testMethod", { key: "hello" });


}
catch(e){console.log(e)}