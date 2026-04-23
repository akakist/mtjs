#pragma once

#include <vector>
#include <stdexcept>
#include <cstring>

extern "C" {
#include <blst.h>
}

namespace blst_cpp {

// -------------------------
// utils
// -------------------------

inline void secure_zero(void* ptr, size_t len) {
    volatile uint8_t* p = reinterpret_cast<volatile uint8_t*>(ptr);
    while (len--) *p++ = 0;
}

// -------------------------
// SecretKey
// -------------------------

class SecretKey {
public:
    SecretKey(const uint8_t* ikm, size_t len) {
        if (!ikm || len == 0)
            throw std::runtime_error("IKM must not be empty");

        blst_keygen(&sk_, ikm, len, nullptr, 0);
    }

    ~SecretKey() {
        secure_zero(&sk_, sizeof(sk_));
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
    explicit PublicKey(const SecretKey& sk) {
        blst_sk_to_pk_in_g1(&pk_, sk.raw());
        blst_p1_to_affine(&pk_affine_, &pk_);
    }

    const blst_p1_affine* affine() const { return &pk_affine_; }

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> out(48);
        blst_p1_affine_compress(out.data(), &pk_affine_);
        return out;
    }

private:
    blst_p1 pk_;
    blst_p1_affine pk_affine_;
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

        if (!msg || msg_len == 0)
            throw std::runtime_error("msg empty");

        blst_hash_to_g2(&sig_, msg, msg_len, dst, dst_len, nullptr, 0);
        blst_sign_pk_in_g1(&sig_, &sig_, sk.raw());

        blst_p2_to_affine(&sig_affine_, &sig_);
    }

    const blst_p2_affine* affine() const { return &sig_affine_; }

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> out(96);
        blst_p2_affine_compress(out.data(), &sig_affine_);
        return out;
    }

    bool verify(const PublicKey& pk,
                const uint8_t* msg,
                size_t msg_len,
                const uint8_t* dst,
                size_t dst_len) const {

        BLST_ERROR err = blst_core_verify_pk_in_g1(
            pk.affine(),
            &sig_affine_,
            true,
            msg,
            msg_len,
            dst,
            dst_len,
            nullptr,
            0
        );

        return err == BLST_SUCCESS;
    }

private:
    blst_p2 sig_;
    blst_p2_affine sig_affine_;
};

// -------------------------
// AggregateSignature (G2)
// -------------------------

class AggregateSignature {
public:
    void add(const Signature& sig) {
        if (!initialized_) {
            blst_p2_from_affine(&agg_, sig.affine());
            initialized_ = true;
        } else {
            blst_p2 tmp;
            blst_p2_from_affine(&tmp, sig.affine());
            blst_p2_add(&agg_, &agg_, &tmp);
        }
    }

    Signature finalize() const {
        if (!initialized_)
            throw std::runtime_error("empty aggregate");

        Signature out;

        blst_p2_affine aff;
        blst_p2_to_affine(&aff, &agg_);

        // hack: создаём через memcpy (без повторной подписи)
        //std::memcpy(&out, &Signature(), sizeof(Signature));
        std::memcpy((void*)&out, &agg_, sizeof(blst_p2));

        return out;
    }

private:
    blst_p2 agg_;
    bool initialized_ = false;
};

} // namespace blst_cpp