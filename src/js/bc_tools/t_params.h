#pragma once
#include "root_contract.h"
#include "md/md_attachment_data.h"
#include "md/md_ValidateBlockREQ.h"

struct b_params
{
    b_params(const REF_getter<root_data>& r): root(r),att_data(new MsgData::attachment_data()) {}
    REF_getter<root_data> root;
    REF_getter<MsgData::ValidateBlockREQ> validateBlockREQ;
    REF_getter<MsgData::attachment_data> att_data;
    BigInt node_rewards;

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


};

struct t_params
{
    t_params(const REF_getter<root_data>& r): root(r) {}
    REF_getter<root_data> root;
    ADDRESS_id senderAddress;
    REF_getter<MsgData::TX> tx;
    EPOCH_id epoch;
    THASH_id tx_id;
    BigInt gasUsed=0;
};