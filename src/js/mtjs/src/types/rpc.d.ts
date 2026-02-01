
    interface rpcListenParams
    {
        ip:string;
        ssl?:boolean;
        ssl_cert?:string;
        ssl_key?:string;
    }
    interface rpcServerRequest
    {
        params: any;
        peer_name: Object;
        sendTo(dst:string, method:string, params:Object): void;
        reply(method:string, params:Object): void;
    };
    interface rpcClientResponse
    {
        params: any;
    };
    interface rpcModule {
        listen(params: rpcListenParams):void;
        sendTo(dst:string, method:string, params:Object): void;
        on_server(method:string, callback: (data:rpcServerRequest) => void): void;
        on_client(method:string, callback: (data:rpcClientResponse) => void): void;
        stop(): void;
    }

