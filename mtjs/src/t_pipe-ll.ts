#!/usr/local/bin/mtjs

try{
  const ee=new mtjs.stream.EventEmitterStream();

  ee.on("data", (data:string) => {
    console.log("dataJS ",data);
  });

  ee.on("end", () => {
    console.log("end");
  });

  ee.on("error", (err:Error) => {
    console.log("error ",err);
  });
  mtjs.pipe.read_lines("ls -l", ee);

}catch (e) {
  console.log("Error: ", e);
}
