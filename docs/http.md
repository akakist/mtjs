# http 
http сделан в простейшей нотации в стике  node.js http module.

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

При разборе хттп запроса определяется тип запроса, в случае `Transfer-Encoding: chunked` выставляется флаг, что запрос is_chunked. Eсли вебсокет, то is_websocket.
В случае если запрос чанкед, то источник данных - стрим, соответственно выход идет тоже в стрим.
Установка стрима делается при обработке первоначального запроса, все последуюшие данные пишутся в стрим.

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
Что делает код? Запускается вебсервер и на него курлом льем поток рандомных байтов.

С вебсокетом пока не доделано, но будет примерно также. Выставляем стрим на приход http заголовка и потом работает только с ним.


