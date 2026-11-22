#pragma once
// Бинарная упаковка значения эмита — тот же формат, что pack_jsi в jsi_pack.cpp:
// [тег][значение], объект — отсортированные ключи. Эмиты больше не JSON-строки.
#include <string>
#include <map>
#include "ioBuffer.h"

class EmitObj
{
public:
    EmitObj& s(const std::string& k, const std::string& v) { V x; x.kind=K_STR; x.s=v; vals_[k]=x; return *this; }
    EmitObj& n(const std::string& k, __int128 v)          { V x; x.kind=K_NUM; x.n=v; vals_[k]=x; return *this; }
    EmitObj& b(const std::string& k, bool v)              { V x; x.kind=K_BOOL; x.b=v; vals_[k]=x; return *this; }

    std::string pack() const
    {
        outBuffer o;
        o.put_8(0);                      // формат контейнера: 0 = CPY (как granule_pack)
        o << (int64_t)5;                 // Object
        o << (int64_t)vals_.size();
        for (auto& kv : vals_) {
            const V& v = kv.second;
            o << kv.first;               // ключ: длина + байты
            if (v.kind == K_STR)      { o << (int64_t)3; o << v.s; }
            else if (v.kind == K_NUM) { o << (int64_t)2; o.put_i128(v.n); }
            else                      { o << (int64_t)1; o << v.b; }
        }
        return o.asString()->container;
    }
private:
    enum Kind { K_STR, K_NUM, K_BOOL };
    struct V { Kind kind; std::string s; __int128 n = 0; bool b = false; };
    std::map<std::string, V> vals_;
};
