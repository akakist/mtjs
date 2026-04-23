#pragma once

template<typename Tag>
struct TypedStr {
    Str value;

    constexpr explicit TypedStr(Str v) : value(v) {}
    constexpr std::string_view view() const {
        return value.view();
    }
};

struct NodeTypeTag {};
struct PkTypeTag {};
struct SigTypeTag {};
struct TxTypeTag {};


using NodeTypeStr = TypedStr<NodeTypeTag>;
using PkTypeStr   = TypedStr<PkTypeTag>;
using SigTypeStr  = TypedStr<SigTypeTag>;
using TxTypeStr   = TypedStr<TxTypeTag>;
