#!/usr/local/bin/mtjs
"use strict";
/*
This is an example of demonstrating telnet; it includes a string calculator.
Note: Inside the runtime, C-style regexps are used, so they have a simplified syntax and are not compatible with JS regexps.
Commands:
ls - show the list of commands.
You can navigate to a directory by typing its name in the console, e.g., type t1.
Inside t1, write expressions like 44*33.
You will receive the result.
*/
try {
    const telnet = mtjs.telnet;
    telnet.listen("localhost:2222", "prod1");
    telnet.set_callback((r) => {
        try {
            const re = /^([0-9\.]+)([\+\-\*\/])([0-9\.]+)$/;
            const res = re.exec(r.command);
            if (res && res.length === 4) {
                const a = parseFloat(res[1]);
                const b = parseFloat(res[3]);
                let result = 0;
                switch (res[2]) {
                    case "+":
                        result = a + b;
                        break;
                    case "-":
                        result = a - b;
                        break;
                    case "*":
                        result = a * b;
                        break;
                    case "/":
                        result = a / b;
                        break;
                    default:
                        console.log("unknown operator ", res[2]);
                        return;
                }
                r.print("result: ", result, "\n");
            }
        }
        catch (err) {
            console.log("error in callback", err);
        }
    });
    telnet.register_command("t1", "^([0-9\\.]+)([\\+-\\*\\/])([0-9\\.]+)$", "calculator");
    mtjs.utils.sleep(1);
    mtjs.asyncExecute("/usr/bin/telnet localhost 2222").then((data) => {
        console.log("telnet done");
    });
}
catch (e) {
    console.log(e);
}
