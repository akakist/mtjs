#!/usr/local/bin/mtjs
try
{
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

  ee.on("error", (err:Error) => {
    console.log("error ",err);
  });
  mtjs.STDIN.read_lines( ee);

}
catch (e) 
{
  console.log("Error: ", e);
}



