#pragma once
#include "root_contract.h"
#include "fee_calcer.h"
struct t_params
{
    t_params(const REF_getter<root_data>& r): root(r) {}
    REF_getter<root_data> root;
    std::vector<std::vector<instruction_report>> instruction_reports;
    std::map<THASH_id,transaction_report> transaction_reports;
    _feeCalcers feeCalcers;

    void logMsg(int txId, int seqId, const char* fmt, ...)
    {

        va_list ap;
        char str[1024];
        va_start(ap, fmt);
        vsnprintf(str, sizeof(str), fmt, ap);
        va_end(ap);
        instruction_reports[txId][seqId].logMsgs.push_back(str);

    }
    void logError(int txId, int seqId, const char* fmt, ...)
    {

        va_list ap;
        char str[1024];
        va_start(ap, fmt);
        vsnprintf(str, sizeof(str), fmt, ap);
        va_end(ap);
        auto& r=instruction_reports[txId][seqId];
        r.err_str=str;
        r.err_code=1;

    }
    void setError(int txId, int seqId,const std::string& err)
    {
        auto& r=instruction_reports[txId][seqId];
        r.err_str=err;
        r.err_code=1;
    }
    void setTxError(THASH_id txHash,const std::string& err)
    {
        auto& r=transaction_reports[txHash];
        r.err_str=err;
        r.err_code=1;

    }
    void setTxSuccess(THASH_id txHash)
    {
        auto& r=transaction_reports[txHash];
        r.err_code=0;

    }

    std::map<std::string, BigInt> fee;

};
