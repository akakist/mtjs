#!/usr/local/bin/mtjs
"use strict";
const opts = {
    //    port: 3306,
    //    host: "localhost",
    user: "root",
    //    password: "123",
    db: "test"
};
try {
    //      const dataFeedId = subscribe.start(opts, (data) => {
    //      console.log("LOG: " + JSON.stringify(data));
    //      });
    // console.log("Data feed ID: ", dataFeedId);
    const mysql = db.mysql_connect(opts);
    mysql.query("create table if not exists test (a int,b int)");
    let n = 0;
    setInterval(() => {
        try {
            n++;
            mysql.query(`insert into test(a,b) values(${n},${n + 1})`);
            if (n % 10 == 0) {
                mysql.query("update test set a=a+1");
            }
            if (n % 20 == 0) {
                mysql.query("delete from test");
            }
            const rsp = mysql.query("SELECT * FROM test");
            console.log("rsp ", JSON.stringify(rsp));
        }
        catch (e) {
            console.log("Error during MySQL operation: ", e);
        }
    }, 1000); // Ping every 5 minutes
    console.log("Connected to MySQL");
    mysql.query("insert into test (a,b) values(1,2)");
    const rsp = mysql.query("SELECT * FROM test");
    console.log("rsp ", JSON.stringify(rsp));
}
catch (e) {
    console.log("Error connecting to MySQL: ", e);
}
