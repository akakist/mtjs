#pragma once
#include "quickjs_i.h"
#include "ioBuffer.h"
#include <string>

// Теги формата гранулы (1 байт в начале). Один формат для гранул, emit и результата query.
enum class JsiTag : int64_t {
    Null   = 0,
    Bool   = 1,
    Int    = 2,   // всегда 128 бит (i128_put: zigzag + 2×PN, 2-10 байт)
    Str    = 3,
    Array  = 4,
    Object = 5,
};

// JSI <-> ioBuffer. Все целые в грануле — 128 бит (тег 2 + i128_put).
std::string pack_jsi(JSIContext* ctx, JSIValueConst val);
JSIValue    unpack_jsi(JSIContext* ctx, const std::string& bytes);

// формат: 0=CPY, 1=ZSTD
int         parse_compression(JSIContext* ctx, JSIValueConst v);
std::string granule_pack(JSIContext* ctx, JSIValueConst val, int compression);
JSIValue    granule_unpack(JSIContext* ctx, const std::string& container);

// BigInt -> outBuffer напрямую (i128_put, без тега)
void        pack_bigint(JSIContext* ctx, JSIValueConst val, outBuffer& o);
