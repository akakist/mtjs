#include "msg.h"
#include <fcntl.h>
#include "cellable.h"
#include "tr_exec.h"
#include "msg_tx.h"
// #include     "nodeService.h"

std::optional<std::string> TR::execute(const tx::registerUser &c, t_params & t,const std::string& senderNick, const REF_getter<fee_calcer>& by, int txid, int seqId)
{
    auto v=t.root->getValues(by);
    auto u=t.root->getUser(senderNick,by);
    if(!u.valid())
        return "user not found/created";
    auto cu=t.root->getUser(c.nickName,by);
    if(cu.valid())
        return "nick already registered";
    cu=t.root->addUser(c.nickName, c.pk, by);
    cu->pkbin=c.pk;
    t.fee[senderNick]+=v->fees[bc_values::register_nick];
    // n=t.root->addNick(c.nickName,by);
    // n->owner=senderAddress;
    t.logMsg(txid,seqId,"user %s sucessfully registered",c.nickName.c_str());

    return std::nullopt;
}

std::optional<std::string> TR::execute(const tx::mint &c, t_params & t,const std::string& senderNick, const REF_getter<fee_calcer>& by, int txid, int seqId)
{
    auto v=t.root->getValues(by);
    auto it=v->emitters.find(senderNick);
    if(it==v->emitters.end())
        return "insufficient_privileges";


    auto u=t.root->getUser(senderNick,by);
    if(!u.valid())
        return "user not found/created";

    t.fee[senderNick]+=v->fees[bc_values::mint];
    t.transfer_to[senderNick]+=c.amount;

    t.logMsg(txid,seqId,"amount %s sucessfully minted on your balance",c.amount.toString().c_str());

    return std::nullopt;
}


std::optional<std::string> TR::execute(const tx::unstake &c, t_params & t,const std::string& senderNick, const REF_getter<fee_calcer>& by, int txid, int seqId)
{
    auto v=t.root->getValues(by);

    auto n=t.root->getNode(c.node, by);
    if(!n.valid())
    {
        return "nodes not registered";
    }
    auto nodeStakeIt = n->stakes.find(senderNick);
    if (nodeStakeIt == n->stakes.end()) {
        return "no_stake_found";
    }
    auto& nodeStake = nodeStakeIt->second;
    if (nodeStake < c.amount) {
        return "insufficient stake";
    }

    // auto v=t.root->getValues(by);

    auto u=t.root->getUser(senderNick,by);


    if(!u.valid())
        return "FATAL:  dst addr not found";


    nodeStake-=c.amount;
    t.transfer_to[senderNick]+=c.amount;
    v->total_staked-=c.amount;

    t.fee[senderNick]+=v->fees[bc_values::unstake];

    t.logMsg(txid,seqId,"node %s unstaked on amount %s",c.node.container.c_str(),c.amount.toString().c_str());

    return std::nullopt;

}
std::optional<std::string> TR::execute(const tx::createContract &c, t_params & t,const std::string& senderNick, const REF_getter<fee_calcer>& by, int txid, int seqId)
{
    auto v=t.root->getValues(by);
    auto u=t.root->getUser(senderNick,by);
    if(!u.valid())
        return "sender not found";


    auto cc=t.root->getContract(c.name,by);
    if(cc.valid())
        return "contract name already exists";


    REF_getter<bc_contract> ci=t.root->addContract(c.name,by);
    ci->owner=senderNick;
    ci->name=c.name;
    ci->src=c.src;
    ci->ready_for_sale=false;
    ci->cost=BigInt(0);

    u->contracts.insert(c.name);

    t.fee[senderNick]+=v->fees[bc_values::contract_deploy];

    t.logMsg(txid,seqId,"contract %s deployed successfully",c.name.c_str());

    return std::nullopt;
}

std::optional<std::string> TR::execute(const tx::transfer &c, t_params & t,const std::string& senderNick, const REF_getter<fee_calcer>& by, int txid, int seqId)
{
    auto v=t.root->getValues(by);
    auto from=t.root->getUser(senderNick,by);
    if(!from.valid())
        return "cannot find src addr";

    REF_getter<bc_user> to(NULL);
    to=t.root->getUser(c.to_nick,by);
    if(!to.valid())
    {
        return "destination user not found "+c.to_nick;
        // to=t.root->addUser(c.to_addr,by);
    }

    t.transfer_from[senderNick]+=c.amount;
    t.transfer_to[c.to_nick]+=c.amount;
    t.fee[senderNick]+=v->fees[bc_values::transfer];
    // // if (from->balance < c.amount + v->fees[bc_values::transfer]) {
    // //     return "insufficient_funds";
    // // }
    // by->add(v->fees[bc_values::transfer]);
    // from->balance-=c.amount;
    // to->balance+=c.amount;
    t.logMsg(txid,seqId,"amount %s successfully transferred",c.amount.toString().c_str());
    return std::nullopt;

}




std::optional<std::string> TR::execute(const tx::stake &c, t_params & t,const std::string& senderNick, const REF_getter<fee_calcer>& by, int txid, int seqId)
{
    auto v=t.root->getValues(by);
    if(!v.valid())
    {
        return "values not found";
    }
    auto n=t.root->getNode(c.node, by);
    if(!n.valid())
    {
        return "Node not registered";
    }
    auto sender = t.root->getUser(senderNick,by);
    if (!sender.valid()) {
        return "user account not found";
    }

    // BigInt amt;
    // amt.from_string(c.amount);
    if (sender->balance+v->fees[bc_values::stake] < c.amount) {
        return "Insufficient funds";
    }

    BigInt& nodeStake = n->stakes[senderNick];

    sender->balance-=c.amount;
    nodeStake+=c.amount;
    t.transfer_from[senderNick]+=c.amount;
    // t.transfer_to[c.to_addr]+=c.amount;
    t.fee[senderNick]+=v->fees[bc_values::stake];

    // v->total_staked+=c.amount;
    // sender->my_stakes[c.node]+=nodeStake;

    // by->add(v->fees[bc_values::stake]);
    t.logMsg(txid,seqId,"node %s staked on amount %s",c.node.container.c_str(),c.amount.toString().c_str());
    return std::nullopt;

}


std::optional<std::string> TR::execute(const tx::registerNode &c, t_params & t,const std::string& senderNick, const REF_getter<fee_calcer>& by, int txid, int seqId)
{
    auto v=t.root->getValues(by);
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
    auto nn=t.root->getNode(c.name, by);
    if(nn.valid())
        return "Node already registered with name";
    auto u=t.root->getUser(senderNick,by);
    if(!u.valid())
        return "if(!u.valid())";
    // if(u->balance<v->fees[bc_values::node_create])
    // return "Not enough funds";

    auto n=t.root->addNode(c.name,by);
    // if(n.valid())
    //     return "fallback, node already registered";

    n->ip=c.ip;
    n->owner=senderNick;
    n->bls_pk=c.pk_bls;
    n->ed_pk=c.pk_ed;
    u->nodes.insert(c.name.container);

    // t.transfer_from[senderAddress]+=c.amount;
    // t.transfer_to[c.to_addr]+=c.amount;
    t.fee[senderNick]+=v->fees[bc_values::node_create];

    // by->add(v->fees[bc_values::node_create]);

    t.logMsg(txid,seqId,"node %s registered",c.name.container.c_str());

    return std::nullopt;

}



