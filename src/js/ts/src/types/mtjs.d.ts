import { StreamModule } from "./stream";

declare global {

    interface userInfo {
        nonce: string;
        balance: string;
    }

    interface mintRequest {
        sk: string;
        amount: string;
        nonce: string;
        timeout: number;
    }

    interface tx_report {
        tx_hash: string;
        errcode: number;
        errstring: string;
        fee: string;
        logMsgs: string[];
    }

    interface AddTxRsp {
        err: number;
        err_str: string;
        tx_hash: string;
    }

    interface Block {
        readonly tx_hashes: string[];
        readonly finalized: boolean;
        readonly block_hash: string;
        readonly block_timestamp: number;
        status(tx_hash: string): "ok" | "err" | "unknown";
        errstr(tx_hash: string): string | null;
        emits(tx_hash: string): [string, unknown][] | null;
        children(tx_hash: string): { [seqId: string]: unknown } | null;
    }
    interface NodeInfo {
        found: boolean;
        name: string;
        owner: string;
        ed_pk: string;
        bls_pk: string;
        ip: string;
        enabled: boolean;
        missed_rounds: number;
        full_stake: string;
        stakes: { [addr: string]: string };
    }
    interface ValuesInfo {
        found: boolean;
        fees: { [k: string]: string };
    }
    interface ContractData {
        found: boolean;
        contract: string;
        key: string;
        value: string;   // JSON-строка, JSON.parse() в тестах
    }
    interface mtjsModule {
        telnet: telnetModule;
        http: httpModule;
        async asyncExecute(command: string): Promise<void>;
        addInstance(name: string, conf: string): void;
        get_user_nonce(nodeaddr: string, addr_hex: string, timeout: number): Promise<userInfo>;
        tx_submit(
            nodeaddr: string,
            timeout: number,
            msg: string,
            sk: string,
            nonce: string,
            callback: (params: object) => void
        ): Promise<AddTxRsp>;
        tx_subscribe(nodeaddr: string, callback: (block: Block) => void): void;
        tx_wait_block(nodeaddr: string, tx_hash_hex: string, timeout: number): Promise<Block>;
        get_block(nodeaddr: string, timeout: number): Promise<Block>;
        get_node_info(nodeaddr: string, name: string, timeout: number): Promise<NodeInfo>;
        get_values(nodeaddr: string, timeout: number): Promise<ValuesInfo>;
        get_contract_data(nodeaddr: string, contract: string, key: string, timeout: number): Promise<ContractData>;
        addr_from_pk(pk: string): string;
        pipe: Pipe;
        STDIN: STDIN;
        rpc: rpcModule;
        stream: StreamModule;
        utils: UtilsModule;
        fs: fsModule;
    }

    const mtjs: mtjsModule;
}

export {};export {  };