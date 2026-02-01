#!/usr/local/bin/mtjs
/*
This example demonstrates reading the pipe from "ls -lR /" 
and forwarding the pipe stream to an emitter, 
which calls a callback with the stream data.
*/
try{
    function arrayBufferToAsciiString(buffer:ArrayBuffer) {
        var bytes = new Uint8Array(buffer);
        var result = '';
        for (var i = 0; i < bytes.length; i++) {
           result += String.fromCharCode(bytes[i]);
        }
        return result;
    }
  const ee=new mtjs.stream.EventEmitterStream();
  ee.on("data", (data:ArrayBuffer) => {
    console.log("dataJS ",arrayBufferToAsciiString(data));
  });
  ee.on("end", () => {
    console.log("end");
  });
  ee.on("error", (er:any) => {
    console.log("error ",er);
  });
  mtjs.pipe.read("ls -lR /", ee);

}catch (e) {
  console.log("Error: ", e);
}
