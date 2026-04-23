import { StreamModule } from "./stream";


export type Request =
    | { type: "stake"; sk: string; node: string; amount:string }
    | { type: "unstake"; sk: string; node: string; amount:string }
    | { type: "create_contract"; sk: string; name: string; amount:string; src: string }
    | { type: "transfer"; to: string; amount:string; src: string }

    // | { type: CreateUser; name: string; email: string }
    // | { type: RequestType.DeleteUser; id: string }
    // | { type: RequestType.GetUser; id: string }
    | { type: "register_user"; nick: string; pk: string  };


declare global
{

    interface userInfo{
        nonce: string;
        balance:string
    }
    interface mintRequest
    {
        sk:string
        amount:string
        nonce: string
        timeout: number
    }
    interface tx_report
    {
        tx_hash: string;
        errcode: number;
        errstring: string;
        fee: string;
        logMsgs: string[];
    }
    interface mtjsModule
    {
        telnet:telnetModule;
        http: httpModule;
        async asyncExecute(command: string): Promise<void>;
        addInstance(name:string, conf:string);
        get_user_info(nodeaddr:string, nick:string, timeout: number): Promise<userInfo>;
        mint(nodeaddr:string, nick:string, params:object): Promise<object>;
        tx_submit(nodeaddr:string, nick:string, nonce:string, sk:string, timeout:number, params:Request[]): Promise<object>;
        tx_subscribe(nodeaddr:string, callback: (params:tx_report) => void): void;
        pipe: Pipe;
        STDIN: STDIN;
        rpc:rpcModule;
        stream:StreamModule;
        utils:UtilsModule;
        fs: fsModule;
    }
    const mtjs: mtjsModule;

}
export {  };