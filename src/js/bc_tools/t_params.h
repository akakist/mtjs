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
    BigInt gasLimit=0;
    BigInt value=0;
    Rollback *roll=NULL;
    void rollback()
    {
        for(auto &z: roll->data)
        {
            inBuffer in(z.second);
            {
                M_LOCK(z.first->mx);
                z.first->unpack_mx(in);
            }
        }
    }
    std::map<NODE_id,REF_getter<bc_node>> nodes;
    REF_getter<bc_node> getNode(const NODE_id& n)
    {
        auto it=nodes.find(n);
        if(it!=nodes.end())
            return it->second;

        auto nn=root->getNode(n);
        if(nn.valid())
        {
            nodes[n]=nn;
            auto p=nn->parent;
            if(!roll->data.count(p))
            {
                M_LOCK(p->mx);
                roll->data[p]=p->getBuffer_mx();
            }
        }
        return nn;
    }
    std::map<ADDRESS_id,REF_getter<bc_address_state>> addrs;
    REF_getter<bc_address_state> getAddressState(const ADDRESS_id& n)
    {
        auto it=addrs.find(n);
        if(it!=addrs.end())
            return it->second;

        auto nn=root->getAddressState(n,roll);
        if(nn.valid())
        {
            addrs[n]=nn;
            auto p=nn->parent;
            if(!roll->data.count(p))
            {
                M_LOCK(p->mx);
                roll->data[p]=p->getBuffer_mx();
            }
        }
        return nn;
    }
};