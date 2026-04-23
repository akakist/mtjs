// blst_cpp.hpp
#pragma once

#include <vector>
#include <stdexcept>
#include <cstring>
#include <memory>
#include <cstdint>

extern "C" {
#include <blst.h>
}

namespace blst_cpp {

constexpr uint8_t BLS_DST[] = "BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_NUL_";
constexpr size_t BLS_DST_LEN = sizeof(BLS_DST) - 1;

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

    const blst_scalar* raw() const { return &sk_; }

private:
    blst_scalar sk_{};
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

    const blst_p1_affine* affine() const {
        if (!affine_cache_) {
            affine_cache_ = std::make_unique<blst_p1_affine>();
            blst_p1_to_affine(affine_cache_.get(), &pk_);
        }
        return affine_cache_.get();
    }

    const blst_p1* raw() const { return &pk_; }

private:
    blst_p1 pk_{};
    mutable std::unique_ptr<blst_p1_affine> affine_cache_;
};

// -------------------------
// Signature (G2)
// -------------------------

class Signature {
public:
    Signature() = default;

    Signature(const SecretKey& sk,
              const uint8_t* msg,
              size_t len)
    {
        // 1) hash_to_g2
        blst_p2 hash;
        blst_hash_to_g2(
            &hash,
            msg, len,
            BLS_DST, BLS_DST_LEN,
            nullptr, 0
        );

        // 2) sign: PK in G1 → SIG in G2
        blst_sign_pk_in_g1(
            &sig_,
            &hash,
            sk.raw()
        );
    }

    const blst_p2_affine* affine() const {
        if (!affine_cache_) {
            affine_cache_ = std::make_unique<blst_p2_affine>();
            blst_p2_to_affine(affine_cache_.get(), &sig_);
        }
        return affine_cache_.get();
    }

    const blst_p2* raw() const { return &sig_; }

private:
    blst_p2 sig_{};
    mutable std::unique_ptr<blst_p2_affine> affine_cache_;
};

// -------------------------
// Single verify
// -------------------------

inline bool verify(const PublicKey& pk,
                   const Signature& sig,
                   const uint8_t* msg,
                   size_t len)
{
    return blst_core_verify_pk_in_g1(
               pk.affine(),
               sig.affine(),
               true,               // hash_to_curve
               msg, len,
               BLS_DST, BLS_DST_LEN,
               nullptr, 0
           ) == BLST_SUCCESS;
}

// -------------------------
// AggregateSignature (one message)
// -------------------------

class AggregateSignature {
public:
    void add(const Signature& s) {
        if (!init_) {
            agg_ = *s.raw();
            init_ = true;
        } else {
            blst_p2_add_or_double(&agg_, &agg_, s.raw());
        }
        affine_cache_.reset();
    }

    const blst_p2_affine* affine() const {
        if (!init_)
            throw std::runtime_error("empty aggregate signature");
        if (!affine_cache_) {
            affine_cache_ = std::make_unique<blst_p2_affine>();
            blst_p2_to_affine(affine_cache_.get(), &agg_);
        }
        return affine_cache_.get();
    }

    const blst_p2* raw() const {
        if (!init_)
            throw std::runtime_error("empty aggregate signature");
        return &agg_;
    }

private:
    blst_p2 agg_{};
    bool init_ = false;
    mutable std::unique_ptr<blst_p2_affine> affine_cache_;
};

// -------------------------
// Fast aggregate verify
// -------------------------

inline bool fast_aggregate_verify(const std::vector<PublicKey>& pks,
                                  const AggregateSignature& agg_sig,
                                  const uint8_t* msg,
                                  size_t len)
{
    if (pks.empty())
        return false;

    blst_p1 agg_pk = *pks[0].raw();
    for (size_t i = 1; i < pks.size(); ++i) {
        blst_p1_add_or_double(&agg_pk, &agg_pk, pks[i].raw());
    }

    blst_p1_affine pk_aff;
    blst_p1_to_affine(&pk_aff, &agg_pk);

    return blst_core_verify_pk_in_g1(
               &pk_aff,
               agg_sig.affine(),
               true,
               msg, len,
               BLS_DST, BLS_DST_LEN,
               nullptr, 0
           ) == BLST_SUCCESS;
}

} // namespace blst_cpp
