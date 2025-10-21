try {
    const http = require("http");
    const serv = http.createServer((req, res) => {
        res.end("<div>received resonse </>");
    });
    serv.listen(8081);
}
catch (e) {
    console.log("error in server " + e);
}