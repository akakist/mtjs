#include "msg.h"
#include "blake2bHasher.h"
thread_local  MsgFactory msgFactory;
MsgData::BlockAcceptedREQ::BlockAcceptedREQ()
    : Base(msgid::BlockAcceptedREQ), leader_certificateZ(new LeaderCertificate()), block_payload(new BlockInfo)
{
}
void MsgData::BlockAcceptedREQ::pack(outBuffer &b) const
{
    XTRY;
    MUTEX_INSPECTOR;
    Base::pack(b);
    leader_certificateZ->pack(b);
    block_payload->pack(b);
    b << node_validators << agg_sig;
    XPASS;
}
void MsgData::BlockAcceptedREQ::unpack(inBuffer &b)
{
    XTRY;
    MUTEX_INSPECTOR;
    Base::unpack(b);
    leader_certificateZ->unpack2(b);
    block_payload->unpack2(b);
    b >> node_validators >> agg_sig;
    XPASS;
}

void MsgData::BlockAcceptedREQ::update(Blake2bHasher &h) const
{
    leader_certificateZ->hash(h);
    block_payload->hash(h);
    for (auto &z : node_validators)
    {
        h.update(z.container);
    }
}

const char *msgName(int id)
{
    switch (id)
    {
    case msgid::node_message_ed:
        return "node_message_ed";
    case msgid::user_message_req:
        return "user_message_req";
    case msgid::transaction_added_rsp:
        return "transaction_added_rsp";
    case msgid::user_request:
        return "user_request";
    case msgid::get_user_status_req:
        return "get_user_status_req";
    case msgid::get_user_status_rsp:
        return "get_user_status_rsp";
    case msgid::HeartBeatREQ:
        return "HeartBeatREQ";
    case msgid::HeartBeatRSP:
        return "HeartBeatRSP";
    case msgid::LeaderCertificate:
        return "LeaderCertificate";
    case msgid::ValidateBlockREQ:
        return "ValidateBlockREQ";
    case msgid::ValidateBlockRSP:
        return "ValidateBlockRSP";
    case msgid::BlockInfo:
        return "BlockInfo";
    case msgid::BlockAcceptedREQ:
        return "BlockAcceptedREQ";
    case msgid::BlockAcceptedRSP:
        return "BlockAcceptedRSP";
    case msgid::GetTransactionREQ:
        return "GetTransactionREQ";
    case msgid::GetTransactionRSP:
        return "GetTransactionRSP";
    case msgid::BlockDBStore:
        return "BlockDBStore";
    case msgid::GetSavedBlocksREQ:
        return "GetSavedBlocksREQ";
    case msgid::GetSavedBlocksRSP:
        return "GetSavedBlocksRSP";
    case msgid::DoHeartBeatREQ:
        return "DoHeartBeatREQ";
    case msgid::ConfirmLeaderREQ:
        return "ConfirmLeaderREQ";
    case msgid::InstructionList:
        return "InstructionList";
    case msgid::TX:
        return "TX";
    case msgid::TxMint:
        return "TxMint";

    default:
        return "unknown";
    }
}
