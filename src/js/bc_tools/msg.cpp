#include "msg.h"
#include "blake2bHasher.h"
#include "md/md_BlockAcceptedREQ.h"

thread_local MsgFactory msgFactory;
MsgData::BlockAcceptedREQ::BlockAcceptedREQ()
    : Base(msgid::BlockAcceptedREQ),
      blockInfo(new BlockInfo)
{
}
void MsgData::BlockAcceptedREQ::pack(outBuffer &b) const
{
    XTRY;
    MUTEX_INSPECTOR;
    Base::pack(b);
    b << blockInfo;
    b << node_validators << agg_sig;
    XPASS;
}
void MsgData::BlockAcceptedREQ::unpack(inBuffer &b)
{
    XTRY;
    MUTEX_INSPECTOR;
    Base::unpack(b);
    b >> blockInfo;
    b >> node_validators >> agg_sig;
    XPASS;
}

void MsgData::BlockAcceptedREQ::update(Blake2bHasher &h) const
{
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
    case msgid::HeartBeatREQ:
        return "HeartBeatREQ";
    case msgid::HeartBeatRSP:
        return "HeartBeatRSP";
    case msgid::ValidateBlockREQ:
        return "ValidateBlockREQ";
    case msgid::ValidateBlockRSP:
        return "ValidateBlockRSP";
    case msgid::BlockInfo:
        return "BlockInfo";
    case msgid::BlockAcceptedREQ:
        return "BlockAcceptedREQ";
    case msgid::GetTransactionREQ:
        return "GetTransactionREQ";
    case msgid::GetTransactionRSP:
        return "GetTransactionRSP";
    case msgid::BlockDBStore:
        return "BlockDBStore";
    case msgid::DoHeartBeatREQ:
        return "DoHeartBeatREQ";
    case msgid::ConfirmLeaderREQ:
        return "ConfirmLeaderREQ";
    case msgid::TX:
        return "TX";
    case msgid::attachment_data:
        return "attachment_data";
    case msgid::GetUserNonceRSP:
        return "GetUserNonceRSP";
    case msgid::GetUserNonceREQ:
        return "GetUserNonceREQ";
    case msgid::LcEnvelopeREQ:
        return "LcEnvelopeREQ";
    case msgid::DelayNotificationREQ:
        return "DelayNotificationREQ";
        

    default:
        return "unknown";
    }
}
