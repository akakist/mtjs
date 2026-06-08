#include "msg.h"
#include "blake2bHasher.h"
#include "md/md_BlockAcceptedREQ.h"

thread_local MsgFactory msgFactory;
MsgData::BlockAcceptedREQ::BlockAcceptedREQ()
    : Base(msgid::BlockAcceptedREQ),
      // leader_certificateZ(new LeaderCertificate()),
      blockInfo(new BlockInfo)
{
}
void MsgData::BlockAcceptedREQ::pack(outBuffer &b) const
{
    XTRY;
    MUTEX_INSPECTOR;
    Base::pack(b);
    // leader_certificateZ->pack(b);
    b << blockInfo;
    b << node_validators << agg_sig;
    XPASS;
}
void MsgData::BlockAcceptedREQ::unpack(inBuffer &b)
{
    XTRY;
    MUTEX_INSPECTOR;
    Base::unpack(b);
    // leader_certificateZ->unpack2(b);
    b >> blockInfo;
    b >> node_validators >> agg_sig;
    XPASS;
}

void MsgData::BlockAcceptedREQ::update(Blake2bHasher &h) const
{
    // leader_certificateZ->update(h);
    blockInfo->update(h);
    for (auto &z : node_validators)
    {
        h.update(z.container);
    }
}

const char *msgName(int id)
{
    switch (id)
    {
    // case msgid::node_message_ed:
    //     return "node_message_ed";
    // case msgid::user_message_req:
    //     return "user_message_req";
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
    case msgid::TX:
        return "TX";
    case msgid::attachment_data:
        return "attachment_data";
    case msgid::GetUserStatusRSP:
        return "GetUserStatusRSP";
    case msgid::GetUserStatusREQ:
        return "GetUserStatusREQ";

    default:
        return "unknown";
    }
}
