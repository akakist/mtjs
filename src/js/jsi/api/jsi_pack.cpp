#include "jsi_pack.h"
#include "ioBuffer.h"
#include "i128.h"
#include <zstd.h>
#include <lz4.h>
#include <map>
#include <stdexcept>
#include <cstring>

static constexpr uint8_t FMT_CPY  = 0;
static constexpr uint8_t FMT_ZSTD = 1;
static constexpr uint8_t FMT_LZ4  = 2;

static bool is_bigint_tag(int t) {
    return t == JSI_TAG_BIG_INT || t == JSI_TAG_SHORT_BIG_INT;
}

// BigInt (JS) -> __int128
static __int128 bigint_to_i128(JSIContext* ctx, JSIValueConst val) {
    size_t len = 0;
    const char* s = JSI_ToCStringLen(ctx, &len, val);  // ToString для BigInt
    if (!s) throw std::runtime_error("bigint: no string");
    std::string str(s, len);
    JSI_FreeCString(ctx, s);
    return i128::fromString(str).v;
}

// __int128 -> BigInt (JS) через глобальный BigInt("...")
static JSIValue i128_to_bigint(JSIContext* ctx, __int128 v) {
    // Нативный QuickJS BigInt (qjsi теперь полный): int64 — быстро, >int64 — литералом.
    if (v >= (__int128)INT64_MIN && v <= (__int128)INT64_MAX)
        return JSI_NewBigInt64(ctx, (int64_t)v);
    std::string src = i128(v).str() + "n";
    JSIValue r = JSI_Eval(ctx, src.c_str(), src.size(), "eval", JSI_EVAL_TYPE_GLOBAL);
    if (JSI_IsException(r)) {
        JSIValue exc = JSI_GetException(ctx);
        const char* es = JSI_ToCString(ctx, exc);
        std::string msg = es ? es : "(no message)";
        if (es) JSI_FreeCString(ctx, es);
        JSI_FreeValue(ctx, exc);
        throw std::runtime_error("BigInt literal '" + src + "' failed: " + msg);
    }
    return r;
}

// ---------- JSI -> outBuffer ----------
// формат: 1 байт тега, int-ы всегда i128 (2 числа zigzag+PN)
static void pack_impl(JSIContext* ctx, JSIValueConst val, outBuffer& o) {
    if (JSI_IsNull(val) || JSI_IsUndefined(val)) { o << (int64_t)JsiTag::Null; return; }
    if (JSI_IsBool(val)) { o << (int64_t)JsiTag::Bool; o << (bool)(JSI_ToBool(ctx, val) != 0); return; }
    int tag = JSI_VALUE_GET_TAG(val);
    if (is_bigint_tag(tag)) {
        o << (int64_t)JsiTag::Int;
        o.put_i128(bigint_to_i128(ctx, val));
        return;
    }
    if (tag == JSI_TAG_INT) {
        // все инты в грануле — 128 бит
        o << (int64_t)JsiTag::Int;
        o.put_i128((__int128)JSI_VALUE_GET_INT(val));
        return;
    }
    if (JSI_IsString(val)) {
        size_t len = 0;
        const char* s = JSI_ToCStringLen(ctx, &len, val);
        if (!s) throw std::runtime_error("bad string");
        std::string r(s, len);
        JSI_FreeCString(ctx, s);
        o << (int64_t)JsiTag::Str;
        o << r;
        return;
    }
    if (JSI_IsArray(ctx, val)) {
        JSIValue lv = JSI_GetPropertyStr(ctx, val, "length");
        int64_t len = 0; JSI_ToInt64(ctx, &len, lv); JSI_FreeValue(ctx, lv);
        o << (int64_t)JsiTag::Array;
        o << (int64_t)len;
        for (int64_t i = 0; i < len; ++i) {
            JSIValue item = JSI_GetPropertyUint32(ctx, val, (uint32_t)i);
            pack_impl(ctx, item, o);
            JSI_FreeValue(ctx, item);
        }
        return;
    }
    if (JSI_IsObject(val)) {
        JSIPropertyEnum* props = nullptr;
        uint32_t count = 0;
        // std::map сортирует по имени — детерминизм (консенсус)
        std::map<std::string, JSIValue> items;
        if (JSI_GetOwnPropertyNames(ctx, &props, &count, val,
                JSI_GPN_STRING_MASK | JSI_GPN_ENUM_ONLY) == 0) {
            for (uint32_t i = 0; i < count; ++i) {
                const char* k = JSI_AtomToCString(ctx, props[i].atom);
                std::string key = k ? k : "";
                if (k) JSI_FreeCString(ctx, k);
                JSIValue item = JSI_GetProperty(ctx, val, props[i].atom);
                items[key] = item;
                JSI_FreeAtom(ctx, props[i].atom);
            }
            jsi_free(ctx, props);
        }
        o << (int64_t)JsiTag::Object;
        o << (int64_t)items.size();
        for (auto& kv : items) {
            o << kv.first;
            pack_impl(ctx, kv.second, o);
            JSI_FreeValue(ctx, kv.second);
        }
        return;
    }
    throw std::runtime_error("unsupported JS value tag=" + std::to_string((int)JSI_VALUE_GET_TAG(val)));
}

// ---------- inBuffer -> JSI ----------
static JSIValue unpack_impl(JSIContext* ctx, inBuffer& in) {
    int64_t tag = 0;
    in >> tag;
    switch (tag) {
        case (int64_t)JsiTag::Null:   return JSI_NULL;
        case (int64_t)JsiTag::Bool: { bool b = false; in >> b; return JSI_NewBool(ctx, b); }
        case (int64_t)JsiTag::Int: { __int128 v = in.get_i128(); return i128_to_bigint(ctx, v); }
        case (int64_t)JsiTag::Str: { std::string s; in >> s; return JSI_NewStringLen(ctx, s.data(), s.size()); }
        case (int64_t)JsiTag::Array: {
            int64_t n = 0; in >> n;
            JSIValue arr = JSI_NewArray(ctx);
            for (int64_t i = 0; i < n; ++i) {
                JSIValue v = unpack_impl(ctx, in);
                JSI_SetPropertyUint32(ctx, arr, (uint32_t)i, v);
            }
            return arr;
        }
        case (int64_t)JsiTag::Object: {
            int64_t n = 0; in >> n;
            JSIValue obj = JSI_NewObject(ctx);
            for (int64_t i = 0; i < n; ++i) {
                std::string k; in >> k;
                JSIValue v = unpack_impl(ctx, in);
                JSI_SetPropertyStr(ctx, obj, k.c_str(), v);
            }
            return obj;
        }
    }
    return JSI_ThrowTypeError(ctx, "unpack_jsi: bad tag");
}

// ---------- zstd ----------
static std::string zstd_compress(const std::string& src) {
    size_t bound = ZSTD_compressBound(src.size());
    std::string dst(bound, '\0');
    size_t r = ZSTD_compress(&dst[0], bound, src.data(), src.size(), 3);
    if (ZSTD_isError(r)) throw std::runtime_error(std::string("zstd: ") + ZSTD_getErrorName(r));
    dst.resize(r);
    return dst;
}
static std::string zstd_decompress(const std::string& src) {
    unsigned long long sz = ZSTD_getFrameContentSize(src.data(), src.size());
    if (sz == ZSTD_CONTENTSIZE_ERROR || sz == ZSTD_CONTENTSIZE_UNKNOWN)
        throw std::runtime_error("zstd: bad frame");
    std::string dst((size_t)sz, '\0');
    size_t r = ZSTD_decompress(&dst[0], dst.size(), src.data(), src.size());
    if (ZSTD_isError(r)) throw std::runtime_error(std::string("zstd: ") + ZSTD_getErrorName(r));
    dst.resize(r);
    return dst;
}

// ---------- lz4 ----------
// lz4 не хранит исходный размер, поэтому формат: [format=LZ4][orig_size(4)][lz4_data]
static std::string lz4_compress_payload(const std::string& src) {
    int bound = LZ4_compressBound((int)src.size());
    if (bound <= 0) throw std::runtime_error("lz4: bad input");
    std::string dst(bound, '\0');
    int n = LZ4_compress_default(src.data(), &dst[0], (int)src.size(), bound);
    if (n <= 0) throw std::runtime_error("lz4: compress failed");
    dst.resize(n);
    uint32_t orig = (uint32_t)src.size();
    std::string out;
    out.append((const char*)&orig, 4);
    out += dst;
    return out;
}
static std::string lz4_decompress_payload(const std::string& payload) {
    if (payload.size() < 4) throw std::runtime_error("lz4: short payload");
    uint32_t orig = 0;
    memcpy(&orig, payload.data(), 4);
    std::string dst(orig, '\0');
    int n = LZ4_decompress_safe(payload.data() + 4, &dst[0], (int)(payload.size() - 4), (int)orig);
    if (n < 0) throw std::runtime_error("lz4: decompress failed");
    dst.resize(n);
    return dst;
}

// ---------- public ----------
std::string pack_jsi(JSIContext* ctx, JSIValueConst val) {
    outBuffer o;
    pack_impl(ctx, val, o);
    return o.asString()->container;
}
JSIValue unpack_jsi(JSIContext* ctx, const std::string& bytes) {
    inBuffer in(bytes);
    return unpack_impl(ctx, in);
}

int parse_compression(JSIContext* ctx, JSIValueConst v) {
    if (JSI_IsUndefined(v) || JSI_IsNull(v)) return FMT_CPY;
    if (!JSI_IsString(v)) throw std::runtime_error("compression must be a string");
    size_t len = 0;
    const char* s = JSI_ToCStringLen(ctx, &len, v);
    std::string c(s, len);
    JSI_FreeCString(ctx, s);
    if (c == "cpy")  return FMT_CPY;
    if (c == "zstd") return FMT_ZSTD;
    if (c == "lz4")  return FMT_LZ4;
    throw std::runtime_error("unknown compression: '" + c + "' (expected cpy|zstd)");
}

// pack_bigint: BigInt -> outBuffer напрямую (i128_put, без тега) — для мест, где тип известен
void pack_bigint(JSIContext* ctx, JSIValueConst val, outBuffer& o) {
    int tag = JSI_VALUE_GET_TAG(val);
    if (is_bigint_tag(tag)) { o.put_i128(bigint_to_i128(ctx, val)); return; }
    if (tag == JSI_TAG_INT) { o.put_i128((__int128)JSI_VALUE_GET_INT(val)); return; }
    throw std::runtime_error("pack_bigint: not an integer");
}

std::string granule_pack(JSIContext* ctx, JSIValueConst val, int compression) {
    std::string packed = pack_jsi(ctx, val);
    std::string out;
    if (compression == FMT_ZSTD)      { out.push_back((char)FMT_ZSTD); out += zstd_compress(packed); }
    else if (compression == FMT_LZ4)  { out.push_back((char)FMT_LZ4);  out += lz4_compress_payload(packed); }
    else                              { out.push_back((char)FMT_CPY);  out += packed; }
    return out;
}
JSIValue granule_unpack(JSIContext* ctx, const std::string& container) {
    if (container.empty()) return JSI_NULL;
    uint8_t fmt = (uint8_t)container[0];
    std::string payload = container.substr(1);
    if (fmt == FMT_CPY)  return unpack_jsi(ctx, payload);
    if (fmt == FMT_ZSTD) return unpack_jsi(ctx, zstd_decompress(payload));
    if (fmt == FMT_LZ4)  return unpack_jsi(ctx, lz4_decompress_payload(payload));
    return JSI_ThrowTypeError(ctx, "granule: unknown format byte %d", (int)fmt);
}
