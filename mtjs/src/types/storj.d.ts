import { StreamModule } from "./stream";

declare global
{

    interface mtjsModule
    {
        telnet:telnetModule;
        http: httpModule;
        async asyncExecute(command: string): Promise<void>;
        pipe: Pipe;
        STDIN: STDIN;
        rpc:rpcModule;
        stream:StreamModule;
        utils:UtilsModule;
    }
    const mtjs: mtjsModule;

}
export {  };