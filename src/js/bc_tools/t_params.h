#pragma once
#include "root_contract.h"
struct t_params
{
    t_params(const REF_getter<root_data>& r): root(r) {}
    REF_getter<root_data> root;
    std::vector<std::vector<instruction_report>> instruction_reports;
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
        instruction_reports[txId][seqId].err_str=str;
        instruction_reports[txId][seqId].err_code=1;

    }
    void setError(int txId, int seqId,const std::string& err)
    {
        instruction_reports[txId][seqId].err_str=err;
        instruction_reports[txId][seqId].err_code=1;

    }

    std::map<std::string, BigInt> transfer_from;
    std::map<std::string, BigInt> transfer_to;
    std::map<std::string, BigInt> fee;

};
