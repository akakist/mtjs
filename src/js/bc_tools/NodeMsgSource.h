#pragma once

struct NodeMsgSource : IMsgSource {
    t_params* t;
    NodeMsgSource(t_params* _t) : t(_t) {}
    std::string get_sender() const override { return base16::encode(t->senderAddress.addr); }
    int64_t get_value() const override { return (int64_t)t->value; }
    int64_t get_gasLimit() const override { return (int64_t)t->gasLimit; }
    int64_t get_gasUsed() const override { return (int64_t)t->gasUsed; }
    int64_t get_blockTimestamp() const override { return (int64_t)t->block_timestamp; }
    int64_t get_blockHeight() const override { return (int64_t)t->block_height; }
    int64_t get_nonce() const override { return (int64_t)t->nonce; }
    int64_t get_chainId() const override { return (int64_t)t->chain_id; }
    std::string get_txHash() const override { return base16::encode(t->tx_id.container); }
    int64_t get_txIndex() const override { return (int64_t)t->tx_index; }
};