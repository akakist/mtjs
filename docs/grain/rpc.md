```markdown
# RPC

RPC in Megatron has a destination address in the form of a host or `local` for local forwarding, as well as a `ServiceId`.

In JS, there is only one `ServiceId` - the service on which JS runs, or more precisely, JS uses the event loop of that service. Therefore, the destination service is always singular. An RPC event consists of two parts: `method` (a string) and `params` (a JSObject). When transmitted over the network, `params` is serialized into JSON and sent as such. At the recipient, it is deserialized into an object and passed to the JS callback in that form. This effectively mimics JSON-RPC 2.0.

There are several RPC samples in the examples:

1. `t_http_rpc.ts` - Starts an HTTP server and an RPC server. It receives an HTTP request, saves it in a map by `request_id`, sends a request via RPC, receives a response, and responds to the earlier HTTP request. This sample demonstrates the design of a typical task where an incoming HTTP request requires fetching data from other nodes, such as a sharded database or similar. Since we can run servers in a single process, everything is done in one script. The HTTP performance on an Ubuntu VM under m4 is 90k rps.
2. `t_rpc2.js` - A client-server pair launched from different processes. The client makes an RPC request and receives a response. The speed is 300k rps.
3. `t_rpc3.js` - Demonstrates routing. A request is sent from the client to a middle node, which forwards it to the server using a route. The server then responds, and the event follows the backroute to reach the client. The key feature of routing here is that the event's route is recorded as it passes through, allowing a reverse event to be sent without manual routing. For example, you can create a dispatcher service that distributes requests to different nodes based on some logic (e.g., load balancing). Responses from the forwarded hosts are routed automatically.

## Main Methods

`listen(params: rpcListenParams): void;` - Listens on a port. Only one RPC can be set per process, but it can listen on multiple ports, so `listen` can be called multiple times with different addresses. This method is used if you want to accept incoming RPC requests.

`sendTo(dst: string, method: string, params: Object): void;` - Sends a JSON-RPC 2.0 request to a host. `dst` is the address in the format 'ip:port'.

`on_server(method: string, callback: (data: rpcServerRequest) => void): void;` - Handles requests on the listening port.

`on_client(method: string, callback: (data: rpcClientResponse) => void): void;` - Handles responses on the client side that sent the request to the server.

### rpcServerRequest
This object has features related to routing.

`sendTo(dst: string, method: string, params: Object): void;` - Forwards a request. This is not a pure forward, as both the method and params need to be specified. It uses the route from `rpcServerRequest`, and the reply will be sent back to the original caller of `rpc.sendTo`.

`reply(method: string, params: Object): void;` - Sends a response. The response goes to the root sender. Forwarders do not receive the response, but physically, the response travels through the forwarding chain.

### rpcClientResponse

This object has no routing features, only `method` and `params`.
```