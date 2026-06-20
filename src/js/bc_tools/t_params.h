#pragma once
#include "root_contract.h"
#include "fee_calcer.h"
#include "md/md_attachment_data.h"
#include "md/md_ValidateBlockREQ.h"
struct t_params
{
    t_params(const REF_getter<root_data>& r): root(r),att_data(new MsgData::attachment_data()) {}
    REF_getter<root_data> root;
    REF_getter<MsgData::ValidateBlockREQ> validateBlockREQ;
    REF_getter<MsgData::attachment_data> att_data;
    _feeCalcers feeCalcers;
    std::map<REF_getter<data_base>, std::set<REF_getter<fee_calcer>> > calcers;
    std::map<ADDRESS_id, BigInt> fee;

    void addCalcer(const REF_getter<data_base>& d,const REF_getter<fee_calcer>& c)
    {
        calcers[d].insert(c);
    }


    void emit_command(const THASH_id& txId, int seqId, const std::string& command, const char* fmt, ...)
    {
        va_list ap;
        char str[1024];
        va_start(ap, fmt);
        int len = vsnprintf(str, sizeof(str), fmt, ap);
        bool overflow=false;
        if (len >= (int)sizeof(str)) {
            overflow=true;
        }
        va_end(ap);
        att_data->blockRoot.children[base16::encode(txId.container)].children[std::to_string(seqId)].emits.push_back({command,overflow?"\"overflow\"":str});
    }
    void emit_tx(const THASH_id& txId, const std::string& command, const char* fmt, ...)
    {
        va_list ap;
        char str[1024];
        va_start(ap, fmt);
        int len = vsnprintf(str, sizeof(str), fmt, ap);
        bool overflow=false;
        if (len >= (int)sizeof(str)) {
            overflow=true;
        }
        va_end(ap);
        att_data->blockRoot.children[base16::encode(txId.container)].emits.push_back({command,overflow?"\"overflow\"":str});
    }
    void emit_block(const std::string& command,const char* fmt, ...)
    {
        va_list ap;
        char str[1024];
        va_start(ap, fmt);
        int len = vsnprintf(str, sizeof(str), fmt, ap);
        bool overflow=false;
        if (len >= (int)sizeof(str)) {
            overflow=true;
        }
        va_end(ap);
        att_data->blockRoot.emits.push_back({command,overflow?"\"overflow\"":str});
    }
    // void logError(const THASH_id& txId, int seqId, const char* fmt, ...)
    // {

    //     va_list ap;
    //     char str[1024];
    //     va_start(ap, fmt);
    //     vsnprintf(str, sizeof(str), fmt, ap);
    //     va_end(ap);
    //     auto& r=att_data->transaction_reports[txId].instruction_reports[seqId];
    //     r.err_str=str;
    //     r.err_code=1;

    // }
    // void setError(const THASH_id& txId, int seqId,const std::string& err)
    // {
    //     auto& r=att_data->transaction_reports[txId].instruction_reports[seqId];
    //     r.err_str=err;
    //     r.err_code=1;
    // }
    // void setTxError(const THASH_id& txHash,const std::string& err)
    // {
    //     auto& r=att_data->transaction_reports[txHash];
    //     r.err_str=err;
    //     r.err_code=1;

    // }
    // void setTxSuccess(const THASH_id& txHash)
    // {
    //     auto& r=att_data->transaction_reports[txHash];
    //     r.err_code=0;

    // }


};
