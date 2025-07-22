declare global 
{

    interface mtjsHttpRequest {
    readonly url: string;
    readonly method: string;
    readonly content: string;
    readonly headers: Object;
    readonly is_chunked: boolean;
    readonly is_websocket: boolean;
    stream: Stream; 
  }
}
declare global 
{
  interface mtjsHttpResponse {
    setHeader(key: string, value: string): Response;
    setHeader(key: Object): Response;
    status(st:number): Response;
    send(...any): Response;
    end(...any): Response;
    write(...any): Response;
  }
}

type HttpRequestListener = (req: mtjsHttpRequest, res: mtjsHttpResponse) => void;

interface WsServerConnection {
  on(eventName: string, callback: (...args: any[]) => void): void;
  send(message: string): void;
}

interface HttpServer {
  listen(port: number, host?: string): void;
  stop():void;
}


interface HttpListenPatams {
  ip?: string; // "0.0.0.0 - aall interfaces"
  port: number; // 8080
  ssl?: boolean; // false
  ssl_cert?: string; // "/path/to/cert.pem"
  ssl_key?: string; // "/path/to/key.pem"
}


interface HttpModule {
  createServer(handler: HttpRequestListener): HttpServer;
}
export {};
