# http 

The HTTP module is implemented in a simple notation styled after the Node.js HTTP module.

```typescript
#!/usr/local/bin/mtjs

try{
    const http=mtjs.http;
    const serv=http.createServer((req:mtjsHttpRequest, res:mtjsHttpResponse) => {
        res.end("<div>received resonse </>");
    }
    );
    serv.listen(6012);

    mtjs.asyncExecute("ab -n 1000000 -k  -c 100   http://127.0.0.1:6012/").then((data:any)=>{
    console.log("ab done", data);
    serv.stop();

});
}
catch(e){console.log("error in server "+e);}
```

When parsing an HTTP request, the request type is determined. If Transfer-Encoding: chunked is present, the is_chunked flag is set. If it's a WebSocket, the is_websocket flag is set. For chunked requests, the data source is a stream, and the output is also written to a stream. The stream is set up during the initial request processing, and all subsequent data is written to the stream.

```typescript
#!/usr/local/bin/mtjs
"use strict";
try {
    const serv = mtjs.http.createServer((req, res) => {
        if (req.is_chunked) {
            let ee = new mtjs.stream.EventEmitterStream;
            ee.on("data", (s) => { console.log("JS on data sz", s.byteLength); res.write("hello world!"); });
            ee.on("end", () => { console.log("JS END"); res.end("hello worldZ!"); });
            req.stream = ee;
        }
        else {
            res.write("<div>received resonse </>");
        }
    });
    serv.listen(6012);
    mtjs.asyncExecute("dd if=/dev/urandom bs=1K count=10000 |   curl -X POST     -H \"Transfer-Encoding: chunked\" -H \"Content-Type: application/octet-stream\"     --data-binary @- http://localhost:6012/")
        .finally(() => serv.stop());
}
catch (e) {
    console.log("error in server " + e);
}

```
What does the code do? It starts a web server, and a stream of random bytes is sent to it using curl.

## websocket

WebSocket support is not yet complete, but it will work similarly. A stream is set up upon receiving the HTTP header, and subsequent operations work with that stream.

## streaming

When a request has `Transfer-Encoding: chunked`, the `request.is_chunked` flag is immediately set. In this case, a stream must be assigned to the request to receive the data. You can set `mtjs.stream.EventEmitterStream`, and the chunks will arrive in the emitter's callbacks.

Accordingly, if you use `send` and `write` in the response, it will produce chunked output. If you use only `end`, the response will include a `Content-Length` header.

