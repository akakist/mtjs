# Streams  

Streams are inspired by express.js.

However, they are modified for better performance. The occupation of workers by asynchronous tasks is avoided. Ideally, there are 2 workers connected via a stream interface, and that's it.

There is no strict division into Readable and Writeable, as one stream is written to by someone and read from by someone else.

The goal of streams is to maximally offload input/output operations from the JS event loop, thereby utilizing the server's other processors, etc.

`ConstReadableStringStream` - creates a buffer from a string that can be read using the stream interface.

`WriteableStringStream` - writes to a large buffer of some stream, for example, input chunked HTTP.

`FileReadableStream` - opens a file for reading and subsequent reading, for example, into a chunked HTTP response.

`FileWriteableStream` - similar, but the other way around.

`EventEmitterStream` - Transfers data to the JS event loop and triggers callbacks.