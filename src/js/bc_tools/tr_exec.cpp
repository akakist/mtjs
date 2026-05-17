#include "msg.h"
#include <fcntl.h>
#include "cellable.h"
#include "tr_exec.h"
#include "msg_tx.h"

std::optional<std::string> TR::execute(const tx::mint &c, t_params & t,const std::string& senderAddress, const REF_getter<fee_calcer>& by, const THASH_id& txid, int seqId)
{
    auto v=t.root->getValues();
    auto it=v->emitters.find(senderAddress);
    if(it==v->emitters.end())
        return "insufficient_privileges";


    auto u=t.root->getUserState(senderAddress);
    if(!u.valid())
    {
        throw CommonError("if(!u.valid())");
    }
    u->balance+=c.amount;
    u->setDirty(by);
    

    t.fee[senderAddress]+=v->fees[bc_values::mint];

    t.logMsg(txid,seqId,"amount %s sucessfully minted on your balance",c.amount.toString().c_str());

    return std::nullopt;
}


std::optional<std::string> TR::execute(const tx::unstake &c, t_params & t,const std::string& senderAddress, const REF_getter<fee_calcer>& by, const THASH_id& txid, int seqId)
{
    auto v=t.root->getValues();

    auto n=t.root->getNode(c.node);
    if(!n.valid())
    {
        return "nodes not registered";
    }
    auto nodeStakeIt = n->stakes.find(senderAddress);
    if (nodeStakeIt == n->stakes.end()) {
        return "no_stake_found";
    }
    auto& nodeStake = nodeStakeIt->second;
    if (nodeStake < c.amount) {
        return "insufficient stake";
    }


    auto u=t.root->getUser(senderAddress);


    if(!u.valid())
        return "FATAL:  dst addr not found";


    nodeStake-=c.amount;
    v->total_staked-=c.amount;
    v->setDirty(by);
    n->setDirty(by);
    u->setDirty(by);

    t.fee[senderAddress]+=v->fees[bc_values::unstake];

    t.logMsg(txid,seqId,"node %s unstaked on amount %s",c.node.container.c_str(),c.amount.toString().c_str());

    return std::nullopt;

}
std::optional<std::string> TR::execute(const tx::createContract &c, t_params & t,const std::string& senderAddress, const REF_getter<fee_calcer>& by, const THASH_id& txid, int seqId)
{
    auto v=t.root->getValues();
    auto u=t.root->getUser(senderAddress);
    if(!u.valid())
        return "sender not found";


    auto cc=t.root->getContract(c.name);
    if(cc.valid())
        return "contract name already exists";

    cc=t.root->addContract(c.name,by);
    
    cc->owner=senderAddress;
    cc->name_=c.name;
    cc->src=c.src;

    u->contracts.insert(c.name);
    u->setDirty(by);
    cc->setDirty(by);
    

    t.fee[senderAddress]+=v->fees[bc_values::contract_deploy];

    t.logMsg(txid,seqId,"contract %s deployed successfully",c.name.c_str());

    return std::nullopt;
}

std::optional<std::string> TR::execute(const tx::transfer &c, t_params & t,const std::string& senderAddress, const REF_getter<fee_calcer>& by, const THASH_id& txid, int seqId)
{
    auto v=t.root->getValues();
    auto from=t.root->getUserState(senderAddress);
    if(!from.valid())
        return "cannot find src addr";

    auto to=t.root->getUserState(c.to_address);
    if(!to.valid())
    {
        return "destination user not found "+ base62::encode(c.to_address);
    }
    if(from->balance < v->fees[bc_values::transfer] + c.amount)
    {
        return "Not enough money";
    }
    from->balance-=c.amount;
    to->balance+=c.amount;
    from->setDirty(by);
    to->setDirty(by);
    t.fee[senderAddress]+=v->fees[bc_values::transfer];
    t.logMsg(txid,seqId,"amount %s successfully transferred",c.amount.toString().c_str());
    return std::nullopt;

}




std::optional<std::string> TR::execute(const tx::stake &c, t_params & t,const std::string& senderAddress, const REF_getter<fee_calcer>& by, const THASH_id& txid, int seqId)
{
    auto v=t.root->getValues();
    if(!v.valid())
    {
        return "values not found";
    }
    auto n=t.root->getNode(c.node);
    if(!n.valid())
    {
        return "Node not registered";
    }
    auto sender = t.root->getUserState(senderAddress);
    if (!sender.valid()) {
        return "user account not found";
    }

    // BigInt amt;
    // amt.from_string(c.amount);
    if (sender->balance+v->fees[bc_values::stake] < c.amount) {
        return "Insufficient funds";
    }

    BigInt& nodeStake = n->stakes[senderAddress];

    sender->balance-=c.amount;
    nodeStake+=c.amount;
    t.fee[senderAddress]+=v->fees[bc_values::stake];

    t.logMsg(txid,seqId,"node %s staked on amount %s",c.node.container.c_str(),c.amount.toString().c_str());
    n->setDirty(by);
    sender->setDirty(by);
    return std::nullopt;

}


std::optional<std::string> TR::execute(const tx::registerNode &c, t_params & t,const std::string& senderAddress, const REF_getter<fee_calcer>& by, const THASH_id& txid, int seqId)
{
    auto v=t.root->getValues();
    for(auto &z: c.name.container)
    {
        if(!isalnum(z))
        {
            return "allowed only isalnum symbols";
        }
        if(isalpha(z) && isupper(z))
        {
            return "allowed only lowercase symbols";
        }
    }
    
    auto nn=t.root->getNode(c.name);
    if(nn.valid())
        return "Node already registered with name";
    auto us=t.root->getUserState(senderAddress);
    if(!us.valid())
        return "if(!us.valid())";
    auto u=t.root->getUser(senderAddress);
    if(!u.valid())
        return "if(!u.valid())";
    if(us->balance < v->fees[bc_values::node_create])
        return "Not enough funds";
    
    u->nodes.insert(c.name);

    auto n=t.root->addNode(c.name,by);
    n->name_=c.name;
    n->ip=c.ip;
    n->ed_pk=c.pk_ed;
    n->bls_pk=c.pk_bls;
    n->owner_ed_pk=senderAddress;
    n->setDirty(by);
    u->setDirty(by);
    us->setDirty(by);


    t.fee[senderAddress]+=v->fees[bc_values::node_create];


    t.logMsg(txid,seqId,"node %s registered",c.name.container.c_str());

    return std::nullopt;

}



