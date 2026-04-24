// blst_cpp.hpp
#pragma once

#include <vector>
#include <stdexcept>
#include <cstdint>
#include <string.h>
// #include "IUtils.h"
#include "ioBuffer.h"
#include "base62.h"
extern "C" {
#include <blst.h>
}

namespace blst_cpp {

    constexpr uint8_t BLS_DST[] = "BLS_SIG_BLS12381G2_XMD:SHA-256_SSWU_RO_NUL_";
    constexpr size_t BLS_DST_LEN = sizeof(BLS_DST) - 1;

    class Signature;
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
        std::string serialize() const {
            std::string out;
            out.resize(32);
            memcpy(out.data(), sk_.b, 32);
            return out;
        }
        void deserializeBase62Str(const std::string& hex) {
            return deserialize(base62::decode(hex));
        }
        void deserialize(const std::string& buf)
        {
            return deserialize((const uint8_t*)buf.data(), buf.size());
        }
        void deserialize(const uint8_t* data, size_t len) {
            if (len != 32)
                throw std::runtime_error("bad sk size");
            memcpy(sk_.b, data, 32);
        }
        // void sign(Signature& sig, const uint8_t* msg,
        //     size_t len);

    private:
        blst_scalar sk_{};
    };

// PublicKey in G1
    class   PublicKey {
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
        std::string serialize() const {
            std::string out;
            out.resize(48);
            blst_p1_compress((uint8_t*)out.data(), &pk_);
            return out;
        }
        void deserializeBase62Str(const std::string& hex) {
            deserialize(base62::decode(hex));
        }
        void deserialize(const std::string& buf)
        {
            deserialize((const uint8_t*)buf.data(), buf.size());
        }
        void deserialize(const uint8_t* data, size_t len) {
            if (len != 48)
                throw std::runtime_error("bad pk(48) size"+std::to_string(len));

            blst_p1_affine aff;
            if (blst_p1_uncompress(&aff, data) != BLST_SUCCESS)
                throw std::runtime_error("bad pk");

            blst_p1_from_affine(&pk_, &aff);
        }
        const blst_p1* raw() const {
            return &pk_;
        }

    private:
        blst_p1 pk_{};
    };

    class Signature;
    inline bool verifyZ(const PublicKey& pk,
                       const Signature& sig,
                       const uint8_t* msg,
                       size_t len);

// Signature in G2
    class Signature {
    public:
        Signature() = default;

        void sign(const SecretKey& sk,
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
        void sign(const SecretKey& sk, const std::string& msg)
        {
            sign(sk, (const uint8_t*)msg.data(), msg.size());
        }

        blst_p2_affine affine() const {
            blst_p2_affine a;
            blst_p2_to_affine(&a, &sig_);
            return a;
        }
        std::string serialize() const {
            std::string out;
            out.resize(96);
            blst_p2_compress((uint8_t*)out.data(), &sig_);
            return out;
        }
        void deserializeBase62Str(const std::string& hex) 
        {
            deserialize(base62::decode(hex));
        }
        void deserialize(const std::string& buf)
        {
            deserialize((const uint8_t*)buf.data(), buf.size());
        }
        void deserialize(const uint8_t* data, size_t len) {
            if (len != 96)
                throw std::runtime_error("bad sig size");

            blst_p2_affine aff;
            if (blst_p2_uncompress(&aff, data) != BLST_SUCCESS)
                throw std::runtime_error("bad sig");

            blst_p2_from_affine(&sig_, &aff);
        }
        bool verify(const PublicKey& pk,
                       const uint8_t* msg,
                       size_t len) const
        {
            return verifyZ(pk, *this, msg, len);
        }
        bool verify(const PublicKey& pk, const std::string& msg) const
        {
            return verifyZ(pk, *this, (const uint8_t*)msg.data(), msg.size());
        }
        const blst_p2* raw() const {
            return &sig_;
        }

    private:
        blst_p2 sig_{};
    };



// Single verify
    inline bool verifyZ(const PublicKey& pk,
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
    class AggregateSignature;
    inline bool fast_aggregate_verify(const std::vector<PublicKey>& pks,
                                      const AggregateSignature& agg_sig,
                                      const uint8_t* msg,
                                      size_t len);

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
        std::string serialize() const {
            if (!init_)
                throw std::runtime_error("empty aggregate signature");

            std::string out;
            out.resize(96);
            blst_p2_compress((byte*)out.data(), &agg_);
            return out;
        }

        // ============================
        // DESERIALIZATION
        // ============================
        void deserializeBase62Str(const std::string& hex) 
        {
            deserialize(base62::decode(hex));
        }
        void deserialize(const std::string& buf)
        {
            deserialize((const uint8_t*)buf.data(), buf.size());
        }
        void deserialize(const uint8_t* data, size_t len) {
            if (len != 96)
                throw std::runtime_error("bad aggregate signature size");

            blst_p2_affine aff;
            if (blst_p2_uncompress(&aff, data) != BLST_SUCCESS)
                throw std::runtime_error("bad aggregate signature");

            blst_p2_from_affine(&agg_, &aff);
            init_ = true;
        }
        const blst_p2* raw() const {
            if (!init_)
                throw std::runtime_error("empty aggregate signature");
            return &agg_;
        }
        bool verify(const std::vector<PublicKey>& pks,const std::string& msg) const
        {
            return fast_aggregate_verify(pks, *this, (const uint8_t*)msg.data(), msg.size());

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


inline outBuffer& operator<<(outBuffer &o,const blst_cpp::SecretKey& z)
{
    o<<z.serialize();
    return o;
}
inline inBuffer& operator>>(inBuffer &o,blst_cpp::SecretKey& z)
{
    std::string s;
    o>>s;
    z.deserialize(s);
    return o;
}
inline outBuffer& operator<<(outBuffer &o,const blst_cpp::PublicKey& z)
{
    o<<z.serialize();
    return o;
}
inline inBuffer& operator>>(inBuffer &o,blst_cpp::PublicKey& z)
{
    std::string s;
    o>>s;
    z.deserialize(s);
    return o;
}
inline outBuffer& operator<<(outBuffer &o,const blst_cpp::Signature& z)
{
    o<<z.serialize();
    return o;
}
inline inBuffer& operator>>(inBuffer &o,blst_cpp::Signature& z)
{
    std::string s;
    o>>s;
    z.deserialize(s);
    return o;
}
inline outBuffer& operator<<(outBuffer &o,const blst_cpp::AggregateSignature& z)
{
    o<<z.serialize();
    return o;
}
inline inBuffer& operator>>(inBuffer &o,blst_cpp::AggregateSignature& z)
{
    std::string s;
    o>>s;
    z.deserialize(s);
    return o;
}
