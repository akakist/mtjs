// blst_cpp.hpp
#pragma once

#include <vector>
#include <stdexcept>
#include <cstdint>
#include <string.h>
extern "C" {
#include <blst.h>
}

namespace blst_cpp {

    constexpr uint8_t BLS_DST[] = "BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_NUL_";
    constexpr size_t BLS_DST_LEN = sizeof(BLS_DST) - 1;

// SecretKey (scalar)
    class SecretKey {
    public:
        SecretKey() = default;

        SecretKey(const uint8_t* ikm, size_t len) {
            if (!ikm || len == 0)
                throw std::runtime_error("IKM empty");
            blst_keygen(&sk_, ikm, len, nullptr, 0);
        }

        const blst_scalar* raw() const {
            return &sk_;
        }
        std::vector<uint8_t> serialize() const {
            std::vector<uint8_t> out(32);
            memcpy(out.data(), sk_.b, 32);
            return out;
        }

        static SecretKey deserialize(const uint8_t* data, size_t len) {
            if (len != 32)
                throw std::runtime_error("bad sk size");
            SecretKey sk;
            memcpy(sk.sk_.b, data, 32);
            return sk;
        }
    private:
        blst_scalar sk_{};
    };

// PublicKey in G1
    class PublicKey {
    public:
        PublicKey() = default;

        explicit PublicKey(const SecretKey& sk) {
            blst_sk_to_pk_in_g1(&pk_, sk.raw());
        }

        blst_p1_affine affine() const {
            blst_p1_affine a;
            blst_p1_to_affine(&a, &pk_);
            return a;
        }
        std::vector<uint8_t> serialize() const {
            std::vector<uint8_t> out(48);
            blst_p1_compress(out.data(), &pk_);
            return out;
        }

        static PublicKey deserialize(const uint8_t* data, size_t len) {
            if (len != 48)
                throw std::runtime_error("bad pk size");

            blst_p1_affine aff;
            if (blst_p1_uncompress(&aff, data) != BLST_SUCCESS)
                throw std::runtime_error("bad pk");

            PublicKey pk;
            blst_p1_from_affine(&pk.pk_, &aff);
            return pk;
        }
        const blst_p1* raw() const {
            return &pk_;
        }

    private:
        blst_p1 pk_{};
    };

// Signature in G2
    class Signature {
    public:
        Signature() = default;

        Signature(const SecretKey& sk,
                  const uint8_t* msg,
                  size_t len)
        {
            blst_p2 hash;
            blst_hash_to_g2(
                &hash,
                msg, len,
                BLS_DST, BLS_DST_LEN,
                nullptr, 0
            );

            blst_sign_pk_in_g1(
                &sig_,
                &hash,
                sk.raw()
            );
        }

        blst_p2_affine affine() const {
            blst_p2_affine a;
            blst_p2_to_affine(&a, &sig_);
            return a;
        }
        std::vector<uint8_t> serialize() const {
            std::vector<uint8_t> out(96);
            blst_p2_compress(out.data(), &sig_);
            return out;
        }

        static Signature deserialize(const uint8_t* data, size_t len) {
            if (len != 96)
                throw std::runtime_error("bad sig size");

            blst_p2_affine aff;
            if (blst_p2_uncompress(&aff, data) != BLST_SUCCESS)
                throw std::runtime_error("bad sig");

            Signature s;
            blst_p2_from_affine(&s.sig_, &aff);
            return s;
        }
        const blst_p2* raw() const {
            return &sig_;
        }

    private:
        blst_p2 sig_{};
    };

// Single verify
    inline bool verify(const PublicKey& pk,
                       const Signature& sig,
                       const uint8_t* msg,
                       size_t len)
    {
        blst_p1_affine pk_aff = pk.affine();
        blst_p2_affine sig_aff = sig.affine();

        return blst_core_verify_pk_in_g1(
                   &pk_aff,
                   &sig_aff,
                   true,
                   msg, len,
                   BLS_DST, BLS_DST_LEN,
                   nullptr, 0
               ) == BLST_SUCCESS;
    }

// AggregateSignature (one message)
    class AggregateSignature {
    public:
        void add(const Signature& s) {
            if (!init_) {
                agg_ = *s.raw();
                init_ = true;
            } else {
                blst_p2_add_or_double(&agg_, &agg_, s.raw());
            }
        }

        blst_p2_affine affine() const {
            if (!init_)
                throw std::runtime_error("empty aggregate signature");
            blst_p2_affine a;
            blst_p2_to_affine(&a, &agg_);
            return a;
        }
        std::vector<uint8_t> serialize() const {
            if (!init_)
                throw std::runtime_error("empty aggregate signature");

            std::vector<uint8_t> out(96);
            blst_p2_compress(out.data(), &agg_);
            return out;
        }

        // ============================
        // DESERIALIZATION
        // ============================
        static AggregateSignature deserialize(const uint8_t* data, size_t len) {
            if (len != 96)
                throw std::runtime_error("bad aggregate signature size");

            blst_p2_affine aff;
            if (blst_p2_uncompress(&aff, data) != BLST_SUCCESS)
                throw std::runtime_error("bad aggregate signature");

            AggregateSignature as;
            blst_p2_from_affine(&as.agg_, &aff);
            as.init_ = true;
            return as;
        }
        const blst_p2* raw() const {
            if (!init_)
                throw std::runtime_error("empty aggregate signature");
            return &agg_;
        }

    private:
        blst_p2 agg_{};
        bool init_ = false;
    };

// Fast aggregate verify (same message)
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

        blst_p2_affine sig_aff = agg_sig.affine();

        return blst_core_verify_pk_in_g1(
                   &pk_aff,
                   &sig_aff,
                   true,
                   msg, len,
                   BLS_DST, BLS_DST_LEN,
                   nullptr, 0
               ) == BLST_SUCCESS;
    }

} // namespace blst_cpp
