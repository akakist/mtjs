#pragma once
#include <optional>
#include "msg.h"
#include "msg_tx.h"
#include "root_contract.h"
#include "t_params.h"

namespace TR {
    std::optional<std::string> execute(const tx::mint &c, t_params & t,const std::string& senderNick, const REF_getter<fee_calcer>& by, int txid, int seqId);
    std::optional<std::string> execute(const tx::unstake &c, t_params & t,const std::string& senderNick, const REF_getter<fee_calcer>& by, int txid, int seqId);
    std::optional<std::string> execute(const tx::createContract &c, t_params & t,const std::string& senderNick, const REF_getter<fee_calcer>& by, int txid, int seqId);
    std::optional<std::string> execute(const tx::transfer &c, t_params & t,const std::string& senderNick, const REF_getter<fee_calcer>& by, int txid, int seqId);
    std::optional<std::string> execute(const tx::stake &c, t_params & t,const std::string& senderNick, const REF_getter<fee_calcer>& by, int txid, int seqId);
    std::optional<std::string> execute(const tx::registerNode &c, t_params & t,const std::string& senderNick, const REF_getter<fee_calcer>& by, int txid, int seqId);
    std::optional<std::string> execute(const tx::transferContract &c, t_params & t,const std::string& senderNick, const REF_getter<fee_calcer>& by, int txid, int seqId);
    std::optional<std::string> execute(const tx::registerUser &c, t_params & t,const std::string& senderNick, const REF_getter<fee_calcer>& by, int txid, int seqId);
}
