#pragma once
#include <blst.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <ioBuffer.h>
static const std::string GRAIN_DST = "GRAIN_BLS_SIG_V1";

// ---------------- Secret Key ----------------

class BlsSecretKey {
    blst_scalar sk;

public:
    static BlsSecretKey fromSeed(const void* seed, size_t len) {
        BlsSecretKey out;
        blst_keygen(&out.sk, (const uint8_t*)seed, len, nullptr, 0);
        return out;
    }

    static BlsSecretKey generate() {
        const char* seed = "grain_default_seed";
        return fromSeed(seed, strlen(seed));
    }

    const blst_scalar* raw() const { return &sk; }
};

outBuffer& operator<<(outBuffer &o,const BlsSecretKey& z);
inBuffer& operator>>(inBuffer &o,BlsSecretKey& z);
// ---------------- Public Key ----------------

class BlsPublicKey {
    blst_p1 pk;

public:
    BlsPublicKey() { memset(&pk, 0, sizeof(pk)); }

    explicit BlsPublicKey(const blst_p1& p) { pk = p; }

    static BlsPublicKey fromSecret(const BlsSecretKey& sk) {
        blst_p1 out;
        blst_sk_to_pk_in_g1(&out, sk.raw());
        return BlsPublicKey(out);
    }

    std::string serialize() const {
        std::string out;
        out.resize(BLST_P1_SERIALIZE_BYTES);
        blst_p1_serialize(out.data(), &pk);
        return out;
    }

    static BlsPublicKey deserialize(const uint8_t* data) {
        blst_p1 out;
        if (blst_p1_deserialize(&out, data) != BLST_SUCCESS)
            throw std::runtime_error("Invalid public key");
        return BlsPublicKey(out);
    }

    const blst_p1* raw() const { return &pk; }
};
outBuffer& operator<<(outBuffer &o,const BlsPublicKey& z);
inBuffer& operator>>(inBuffer &o,BlsPublicKey& z);

// ---------------- Signature ----------------

class BlsSignature {
    blst_p2 sig;

public:
    BlsSignature() { memset(&sig, 0, sizeof(sig)); }

    explicit BlsSignature(const blst_p2& s) { sig = s; }

    static BlsSignature sign(const BlsSecretKey& sk, const void* msg, size_t len) {
        blst_p2 out;
        blst_sign_pk_in_g2(
            &out,
            (const uint8_t*)msg, len,
            (const uint8_t*)GRAIN_DST.data(), GRAIN_DST.size(),
            sk.raw()
        );
        return BlsSignature(out);
    }

    bool verify(const BlsPublicKey& pk, const void* msg, size_t len) const {
        BLST_ERROR err = blst_core_verify_pk_in_g1(
            pk.raw(),
            &sig,
            1, // hash_to_curve
            (const uint8_t*)msg, len,
            (const uint8_t*)GRAIN_DST.data(), GRAIN_DST.size(),
            nullptr, 0
        );
        return err == BLST_SUCCESS;
    }

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> out(BLST_P2_SERIALIZE_BYTES);
        blst_p2_serialize(out.data(), &sig);
        return out;
    }

    static BlsSignature deserialize(const uint8_t* data) {
        blst_p2 out;
        if (blst_p2_deserialize(&out, data) != BLST_SUCCESS)
            throw std::runtime_error("Invalid signature");
        return BlsSignature(out);
    }

    const blst_p2* raw() const { return &sig; }
};
outBuffer& operator<<(outBuffer &o,const BlsSignature& z);
inBuffer& operator>>(inBuffer &o,BlsSignature& z);

// ---------------- Aggregate Signature ----------------

class BlsAggregateSignature {
    blst_p2 agg;

public:
    BlsAggregateSignature() { memset(&agg, 0, sizeof(agg)); }

    void add(const BlsSignature& s) {
        if (blst_p2_is_inf(&agg)) {
            agg = *s.raw();
        } else {
            blst_p2_add_or_double(&agg, s.raw());
        }
    }
    bool verify(const std::vector<BlsPublicKey>& pks, const std::string& msg) const 
    {
        return verify(pks, msg.data(), msg.size());
    }

    bool verify(const std::vector<BlsPublicKey>& pks, const void* msg, size_t len) const {
        // Aggregate public keys
        blst_p1 agg_pk;
        memset(&agg_pk, 0, sizeof(agg_pk));

        for (size_t i = 0; i < pks.size(); i++) {
            if (i == 0)
                agg_pk = *pks[i].raw();
            else
                blst_p1_add_or_double(&agg_pk, pks[i].raw());
        }

        BLST_ERROR err = blst_core_verify_pk_in_g1(
            &agg_pk,
            &agg,
            1,
            (const uint8_t*)msg, len,
            (const uint8_t*)GRAIN_DST.data(), GRAIN_DST.size(),
            nullptr, 0
        );

        return err == BLST_SUCCESS;
    }

    const blst_p2* raw() const { return &agg; }
};

outBuffer& operator<<(outBuffer &o,const BlsAggregateSignature& z);
inBuffer& operator>>(inBuffer &o,BlsAggregateSignature& z);
