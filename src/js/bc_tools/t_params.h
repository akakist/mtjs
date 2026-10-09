#pragma once
#include "md/md_attachment_data.h"
#include "md/md_ValidateBlockREQ.h"
#include "IDatabase.h"
#include "blockMeta.h"
struct b_params
{
    b_params(IDatabase* _db): att_data(new MsgData::attachment_data()),db(_db) {}
    IDatabase* db;
    REF_getter<MsgData::ValidateBlockREQ> validateBlockREQ;
    REF_getter<MsgData::attachment_data> att_data;
    uint64_t node_rewards;
    REF_getter<BlockMetaFull> meta;
    // std::map<NODE_id, std::map<char,uint64_t>> _node_stake_changes;
    // std::map<NODE_id, int> _node_enables;

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
    t_params(const REF_getter<IDatabase>& d)
    : 
    db(d) 
    {}
    REF_getter<IDatabase> db;
    ADDRESS_id senderAddress;
    REF_getter<MsgData::TX> tx;
    uint64_t epoch;
    THASH_id tx_id;
    uint64_t gasUsed=0;
    uint64_t gasLimit=0;
    uint64_t value=0;
    Rollback *roll=NULL;
    Dirty dirty;
    // std::map<NODE_id, std::map<char,uint64_t>> node_stake_changes;
    // std::map<NODE_id, int> node_enables;

    void markDirty(data_base *p)
    {
        p->setDirty(roll);
        dirty.add(p);
    }
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
    // std::map<NODE_id,REF_getter<bc_node>> nodes;
    // REF_getter<bc_node> getNode_(const NODE_id& n)
    // {
    //     auto it=nodes.find(n);
    //     if(it!=nodes.end())
    //         return it->second;

    //     auto nn=db->getNodeNoCreate(n);

    //     if(nn.valid())
    //     {
    //         nodes[n]=nn;
    //         markDirty(nn.get());
    //         // nn->setDirty(roll);
    //         // dirty.add(nn.get());

    //     }
    //     return nn;
    // }
    // std::map<ADDRESS_id,REF_getter<bc_address_state>> addrs;
    // REF_getter<bc_address_state> getAddressState(const ADDRESS_id& n)
    // {
    //     auto it=addrs.find(n);
    //     if(it!=addrs.end())
    //         return it->second;

    //     auto nn=db->getAddressStateOrCreate(n,roll);
    //     if(nn.valid())
    //     {
    //         addrs[n]=nn;
    //         markDirty(nn.get());
    //         // nn->setDirty(roll);
    //         // dirty.add(nn.get());
    //     }
    //     return nn;
    // }
};