#pragma once
#include "root_contract.h"
#include "fee_calcer.h"
#include "md/md_attachment_data.h"
struct t_params
{
    t_params(const REF_getter<root_data>& r): root(r),att_data(new MsgData::attachment_data()) {}
    REF_getter<root_data> root;
    // std::vector<std::vector<instruction_report>> instruction_reports;
    // std::map<THASH_id,transaction_report> transaction_reports;
    REF_getter<MsgData::attachment_data> att_data;
    _feeCalcers feeCalcers;


    void logMsg(const THASH_id& txId, int seqId, const char* fmt, ...)
    {

        va_list ap;
        char str[1024];
        va_start(ap, fmt);
        vsnprintf(str, sizeof(str), fmt, ap);
        va_end(ap);
        att_data->transaction_reports[txId].instruction_reports[seqId].logMsgs.push_back(str);

    }
    void logError(const THASH_id& txId, int seqId, const char* fmt, ...)
    {

        va_list ap;
        char str[1024];
        va_start(ap, fmt);
        vsnprintf(str, sizeof(str), fmt, ap);
        va_end(ap);
        auto& r=att_data->transaction_reports[txId].instruction_reports[seqId];
        r.err_str=str;
        r.err_code=1;

    }
    void setError(const THASH_id& txId, int seqId,const std::string& err)
    {
        auto& r=att_data->transaction_reports[txId].instruction_reports[seqId];
        r.err_str=err;
        r.err_code=1;
    }
    void setTxError(const THASH_id& txHash,const std::string& err)
    {
        auto& r=att_data->transaction_reports[txHash];
        r.err_str=err;
        r.err_code=1;

    }
    void setTxSuccess(const THASH_id& txHash)
    {
        auto& r=att_data->transaction_reports[txHash];
        r.err_code=0;

    }

    std::map<std::string, BigInt> fee;

};
