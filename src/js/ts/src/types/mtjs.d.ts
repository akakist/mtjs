import { StreamModule } from "./stream";

declare global {
interface KeyPair {
    type: number;   // 0 = ed25519, 1 = secp256k1, 2 = p256, 3 = bls12-381
    sk: string;     // hex (ed25519: 64, ecdsa: 32, bls: 32)
    pk: string;     // hex (ed25519: 32, ecdsa: 33 compressed, bls: 48 compressed)
}

    interface ScanResult { dbids: string[]; next_from: string; done: boolean; scanned: number; }
    interface Granule {
        found: boolean;
        stale: boolean;     // этап 2; сейчас всегда false
        body: string;       // тело гранулы hex (JS не умеет бинарные строки)
    }
    interface CallViewResult {
        found: boolean;
        error: boolean;
        result: string;   // JSON-строка
    }
    interface AddressKey { id: number; type: number; keybuffer: string; }

    interface AddressInfo {
        found: boolean;
        balance: string;
        nonce: string;
        keys: AddressKey[];
    }

    interface StateInfo {
        found: boolean;
        total_supply: string;
        total_stake: string;
        account_count: number;
        node_count: number;
        contract_count: number;
        tx_count: number;
        block_height: number;
        genesis_time: number;
        chain_id: number;
    }

    interface ContractInfo {
        found: boolean;
        contract: string;
        owners: string[];
        src: string;
        ttl: number;
        update_time: number;
    }
    interface userInfo {
        nonce: string;
        balance: string;
        found: bool;
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
        answered_rounds: number;
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
        value: string;
    }
    interface mtjsModule {
        // address
        addr_from_name(nick: string): string;
        get_address_info(nodeaddr: string, addr_hex: string, timeout: number): Promise<AddressInfo>;
        get_granule(nodeaddr: string, addr_hex: string, hash_hex: string, timeout: number): Promise<Granule>;
        get_granule_lite(addr_hex: string, hash_hex: string, timeout: number): Promise<Granule>;
	scan_expired(nodeaddr: string, from_hex: string, max_sec: number): Promise<ScanResult>;

        // mtjs core
        telnet: telnetModule;
        http: httpModule;
        async asyncExecute(command: string): Promise<void>;
        addInstance(name: string, conf: string): string;
        killInstance(handle: string): void;
        get_user_nonce(nodeaddr: string, addr_hex: string, timeout: number): Promise<userInfo>;
        tx_submit(
            nodeaddr: string,
            timeout: number,
            msg: string,
            sk: string,
            nonce: string,
            callback: (params: object) => void
        ): Promise<AddTxRsp>;

        generate_keypair(type?: number): KeyPair;
        tx_subscribe(nodeaddr: string, callback: (block: Block) => void): void;
        tx_wait_block(nodeaddr: string, tx_hash_hex: string, timeout: number): Promise<Block>;
        get_block(nodeaddr: string, timeout: number): Promise<Block>;
        get_node_info(nodeaddr: string, name: string, timeout: number): Promise<NodeInfo>;
        get_values(nodeaddr: string, timeout: number): Promise<ValuesInfo>;
        get_contract_data(nodeaddr: string, contract: string, key: string, timeout: number): Promise<ContractData>;
        get_contract(nodeaddr: string, contract: string, timeout: number): Promise<ContractInfo>;
        get_state(nodeaddr: string, timeout: number): Promise<StateInfo>;
        call_view(nodeaddr: string, contract: string, view: string, params: object, timeout_sec: number, timeout: number): Promise<CallViewResult>;
        tx_unsubscribe(nodeaddr: string, tx_hash_hex?: string): void;
	    lite_set_uplink(addr: string): void;
        blake2b_hex(str: string): string;
        sign_ed(sk_hex: string, msg: string): string;
        pipe: Pipe;
        STDIN: STDIN;
        rpc: rpcModule;
        stream: StreamModule;
        utils: UtilsModule;
        fs: fsModule;
    }

    const mtjs: mtjsModule;
}

export {};