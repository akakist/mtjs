#pragma once
#include <string>
#include <cstdint>

#pragma once
#include <string>
#include <cstdint>

struct IMsgSource {
    virtual ~IMsgSource() = default;
    virtual std::string get_sender() const = 0;       // hex (нода) / "alice" (test_runner)
    virtual int64_t get_value() const = 0;
    virtual int64_t get_gasLimit() const = 0;
    virtual int64_t get_gasUsed() const = 0;
    virtual int64_t get_blockTimestamp() const = 0;
    virtual int64_t get_blockHeight() const = 0;
    virtual int64_t get_nonce() const = 0;
    virtual int64_t get_chainId() const = 0;
    virtual std::string get_txHash() const = 0;        // hex
    virtual int64_t get_txIndex() const = 0;

    // value: явное списание/возврат (consumeValue/refundValue в JSI)
    virtual void consume_value(int64_t amount) = 0;    // t->value -= amount (guard — в JSI)
    virtual void refund_value(int64_t amount) = 0;     // t->value += amount
    virtual void consume_gas(int64_t amount) = 0;      // gasUsed += amount (interrupt_handler)
};

