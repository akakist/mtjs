#pragma once

#include <vector>
#include <stdexcept>
#include <cstring>

extern "C" {
#include <blst.h>
}

#include <blst.hpp>

namespace blst_cpp {

// -------------------------
// SecretKey
// -------------------------

class SecretKey {
public:
    SecretKey() = default;

    SecretKey(const uint8_t* ikm, size_t len) {
        if (!ikm || len == 0)
            throw std::runtime_error("IKM empty");

        blst_keygen(&sk_, ikm, len, nullptr, 0);
    }

std::vector<uint8_t> serialize() const {
    std::vector<uint8_t> out(32);
    std::memcpy(out.data(), &sk_, 32);
    return out;
}
    static SecretKey deserialize(const uint8_t* data, size_t len) {
        if (len != 32)
            throw std::runtime_error("bad sk size");

        SecretKey sk;
        std::memcpy(&sk.sk_, data, 32);
        return sk;
    }

    const blst_scalar* raw() const { return &sk_; }

private:
    blst_scalar sk_;
};

// -------------------------
// PublicKey (G1)
// -------------------------

class PublicKey {
public:
    PublicKey() = default;

    explicit PublicKey(const SecretKey& sk) {
        blst_sk_to_pk_in_g1(&pk_, sk.raw());
    }

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> out(48);
        blst_p1_compress(out.data(), &pk_);
        return out;
    }

    static PublicKey deserialize(const uint8_t* data, size_t len) {
        if (len != 48)
            throw std::runtime_error("bad pk size");

        PublicKey pk;
        blst_p1_affine aff;

        if (blst_p1_uncompress(&aff, data) != BLST_SUCCESS)
            throw std::runtime_error("bad pk");

        blst_p1_from_affine(&pk.pk_, &aff);
        return pk;
    }

    const blst_p1* raw() const { return &pk_; }

private:
    blst_p1 pk_;
};

// -------------------------
// Signature (G2)
// -------------------------

class Signature {
public:
    Signature() = default;

    Signature(const SecretKey& sk,
              const uint8_t* msg,
              size_t msg_len,
              const uint8_t* dst,
              size_t dst_len) {

        blst_p2 hash;
        blst_hash_to_g2(&hash, msg, msg_len, dst, dst_len, nullptr, 0);
        blst_sign_pk_in_g1(&sig_, &hash, sk.raw());
    }

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> out(96);
        blst_p2_compress(out.data(), &sig_);
        return out;
    }

    static Signature deserialize(const uint8_t* data, size_t len) {
        if (len != 96)
            throw std::runtime_error("bad sig size");

        Signature s;
        blst_p2_affine aff;

        if (blst_p2_uncompress(&aff, data) != BLST_SUCCESS)
            throw std::runtime_error("bad sig");

        blst_p2_from_affine(&s.sig_, &aff);
        return s;
    }

    const blst_p2* raw() const { return &sig_; }

private:
    blst_p2 sig_;
};

// -------------------------
// AggregateSignature
// -------------------------

class AggregateSignature {
public:
    void add(const Signature& sig) {
        if (!init_) {
            agg_ = *sig.raw();
            init_ = true;
        } else {
            blst_p2_add(&agg_, &agg_, sig.raw());
        }
    }

    Signature finalize() const {
        if (!init_)
            throw std::runtime_error("empty agg");

        Signature s;
        std::memcpy(&s, &agg_, sizeof(blst_p2)); // safe: same layout start
        return s;
    }

    const blst_p2* raw() const { return &agg_; }

private:
    blst_p2 agg_;
    bool init_ = false;
};

// -------------------------
// FAST AGG VERIFY
// -------------------------
/*
inline bool fast_aggregate_verify(
    const AggregateSignature& agg_sig,
    const std::vector<PublicKey>& pks,
    const uint8_t* msg,
    size_t msg_len,
    const uint8_t* dst,
    size_t dst_len
) {
    if (pks.empty())
        return false;

    blst_p1 agg_pk = *pks[0].raw();

    for (size_t i = 1; i < pks.size(); i++) {
        blst_p1_add(&agg_pk, &agg_pk, pks[i].raw());
    }

    blst_p1_affine pk_aff;
    blst_p2_affine sig_aff;

    blst_p1_to_affine(&pk_aff, &agg_pk);
    blst_p2_to_affine(&sig_aff, agg_sig.raw());

    return blst_core_verify_pk_in_g1(
        &pk_aff,
        &sig_aff,
        true,
        msg,
        msg_len,
        dst,
        dst_len,
        nullptr,
        0
    ) == BLST_SUCCESS;
}
*/

inline bool aggregate_verify(
    const blst_cpp::AggregateSignature& agg_sig,
    const std::vector<blst_cpp::PublicKey>& pks,
    const uint8_t* msg,
    size_t msg_len,
    const uint8_t* dst,
    size_t dst_len
) {
    if (pks.empty())
        return false;

    blst::Pairing ctx(true, dst, dst_len);

    for (const auto& pk : pks) {
        ctx.aggregate(
            pk.raw(),
            true,
            msg,
            msg_len
        );
    }
    return ctx.finalverify(agg_sig.finalize().raw()) == BLST_SUCCESS;
//    return ctx.finalverify(agg_sig.raw()) 
}
} // namespace blst_cpp