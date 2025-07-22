try {
    const http = require("http");
    const serv = http.createServer((req, res) => {
        res.end("<div>received resonse </>");
    });
    serv.listen(8080);
}
catch (e) {
    console.log("error in server " + e);
}