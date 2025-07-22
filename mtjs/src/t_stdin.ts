#!/usr/local/bin/mtjs

mtjs.asyncExecute("ls |./stdin.js").then((data:any) => {
    console.log("job done");
});
