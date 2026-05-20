#include "msgFactory.h"
#include "msg.h"
MsgFactory::MsgFactory() 
{
    registerMsg(msgid::HeartBeatREQ, MsgData::HeartBeatREQ::construct);
    registerMsg(msgid::HeartBeatRSP, MsgData::HeartBeatRSP::construct);
    registerMsg(msgid::GetTransactionREQ, MsgData::GetTransactionREQ::construct);
    registerMsg(msgid::GetTransactionRSP, MsgData::GetTransactionRSP::construct);
    registerMsg(msgid::ValidateBlockREQ, MsgData::ValidateBlockREQ::construct);
    registerMsg(msgid::ValidateBlockRSP, MsgData::ValidateBlockRSP::construct);
    registerMsg(msgid::BlockAcceptedREQ, MsgData::BlockAcceptedREQ::construct);
    registerMsg(msgid::BlockAcceptedRSP, MsgData::BlockAcceptedRSP::construct);
    registerMsg(msgid::GetSavedBlocksREQ, MsgData::GetSavedBlocksREQ::construct);
    registerMsg(msgid::GetSavedBlocksRSP, MsgData::GetSavedBlocksRSP::construct);
    registerMsg(msgid::DoHeartBeatREQ, MsgData::DoHeartBeatREQ::construct);
    registerMsg(msgid::ConfirmLeaderREQ, MsgData::ConfirmLeaderREQ::construct);
    registerMsg(msgid::ConfirmLeaderRSP, MsgData::ConfirmLeaderRSP::construct);
    registerMsg(msgid::InstructionList, MsgData::InstructionList::construct);
    registerMsg(msgid::TX, MsgData::TX::construct);
    registerMsg(msgid::TxMint, MsgData::TxMint::construct);

}
