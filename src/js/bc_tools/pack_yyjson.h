#pragma once
// yyjson -> бинарный pack_jsi (тег/значение, ключи отсортированы) — без байта формата,
// как pack_jsi: его читает unpack_jsi.
// Нужен, чтобы передавать параметры контрактного вызова между JSI-контекстами
// без JSON и без nlohmann.
#include <string>
#include <vector>
#include <algorithm>
#include <utility>
#include "yyjson.h"
#include "ioBuffer.h"

namespace pak {

inline void put_str(outBuffer& o, const std::string& s) { o << (int64_t)3; o << s; }
inline void put_int(outBuffer& o, __int128 v)            { o << (int64_t)2; o.put_i128(v); }
inline void put_bool(outBuffer& o, bool b)              { o << (int64_t)1; o << b; }

inline void pack_val(outBuffer& o, yyjson_val* v)
{
    if (!v || yyjson_is_null(v)) { o << (int64_t)0; return; }
    if (yyjson_is_bool(v)) { put_bool(o, yyjson_get_bool(v)); return; }
    if (yyjson_is_str(v))  { put_str(o, std::string(yyjson_get_str(v), yyjson_get_len(v))); return; }
    if (yyjson_is_uint(v)) { put_int(o, (__int128)yyjson_get_uint(v)); return; }
    if (yyjson_is_int(v))  { put_int(o, (__int128)yyjson_get_sint(v)); return; }
    if (yyjson_is_real(v)) {
        double d = yyjson_get_real(v);
        if (d == (double)(int64_t)d) { put_int(o, (__int128)(int64_t)d); return; }
        throw std::runtime_error("pack_yyjson: non-integer number is not supported");
    }
    if (yyjson_is_arr(v)) {
        o << (int64_t)4; o << (int64_t)yyjson_arr_size(v);
        yyjson_arr_iter it = yyjson_arr_iter_with(v);
        yyjson_val* e;
        while ((e = yyjson_arr_iter_next(&it))) pack_val(o, e);
        return;
    }
    if (yyjson_is_obj(v)) {
        std::vector<std::pair<std::string, yyjson_val*>> kv;
        yyjson_obj_iter it = yyjson_obj_iter_with(v);
        yyjson_val* k;
        while ((k = yyjson_obj_iter_next(&it))) {
            kv.emplace_back(std::string(yyjson_get_str(k), yyjson_get_len(k)),
                            yyjson_obj_iter_get_val(k));
        }
        std::sort(kv.begin(), kv.end(),
                  [](const std::pair<std::string, yyjson_val*>& a,
                     const std::pair<std::string, yyjson_val*>& b) { return a.first < b.first; });
        o << (int64_t)5; o << (int64_t)kv.size();
        for (auto& e : kv) { o << e.first; pack_val(o, e.second); }
        return;
    }
    o << (int64_t)0;
}

inline std::string pack_yyjson(yyjson_val* v)
{
    outBuffer o;
    pack_val(o, v);
    return o.asString()->container;
}

} // namespace pak
