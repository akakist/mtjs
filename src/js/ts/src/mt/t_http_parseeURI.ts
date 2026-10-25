#!/usr/local/bin/mtjs

/*
This example listens on HTTP port 6012 and initiates a file upload from /tmp/z using curl. Demonstrates chunked transfer encoding.

*/
try{
    const serv=mtjs.http.createServer((req:mtjsHttpRequest, res:mtjsHttpResponse) => {
	const p=req.parseURI();
	const h=req.parseHeaders();
	console.log(JSON.stringify(p));
	console.log(JSON.stringify(h));
    res.end("7308729830928");
    }
    );
serv.listen(6012);

 mtjs.asyncExecute("curl      http://localhost:6012/zaza/oaoa/eee?aa=33\\&bb=33")
//mtjs.asyncExecute("curl -X POST     -H \"Transfer-Encoding: chunked\" -H \"Content-Type: application/octet-stream\"     --data-binary @/tmp/z http://localhost:6012/")
.finally(()=>serv.stop());


}
catch(e){console.log("error in server "+e);}


