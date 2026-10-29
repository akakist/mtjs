// Зеркало jsi/runner/test_runner.cpp (TestCase/TxCommand/Transaction/jsiEvent).
// .tx.ts — expression `({ ... } as TestTx);` → tsc → `({ ... });` (JSI-Eval).

interface JsiEvent { contract?: string; name?: string; value?: string; }

interface TxCommand {
    contract: string;
    method: string;
    params: object;
    expect_error?: boolean;
    expect_result?: any;
    expect_events?: JsiEvent[];
}

interface TxMsg {
    sender?: string;
    value?: number;
    gasLimit?: number;
    blockTimestamp?: number;
    blockHeight?: number;
    nonce?: number;
    chainId?: number;
    txHash?: string;
    txIndex?: number;
}

interface Tx {
    msg?: TxMsg;
    commands: TxCommand[];
}

interface TestTx {
    time?: number;
    initial?: object;
    transactions: Tx[];
    expected_emits?: JsiEvent[];
    expected?: object;
}
