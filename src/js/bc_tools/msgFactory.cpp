#include "msgFactory.h"
#include "msg.h"
MsgFactory::MsgFactory() 
{
    registerMsg(msgid::HeartBeatREQ, MsgEvt::HeartBeatREQ::construct);
    registerMsg(msgid::HeartBeatRSP, MsgEvt::HeartBeatRSP::construct);
    registerMsg(msgid::GetTransactionREQ, MsgEvt::GetTransactionREQ::construct);
    registerMsg(msgid::GetTransactionRSP, MsgEvt::GetTransactionRSP::construct);
    registerMsg(msgid::ValidateBlockREQ, MsgEvt::ValidateBlockREQ::construct);
    registerMsg(msgid::ValidateBlockRSP, MsgEvt::ValidateBlockRSP::construct);
    registerMsg(msgid::BlockAcceptedREQ, MsgEvt::BlockAcceptedREQ::construct);
    registerMsg(msgid::BlockAcceptedRSP, MsgEvt::BlockAcceptedRSP::construct);
    registerMsg(msgid::GetSavedBlocksREQ, MsgEvt::GetSavedBlocksREQ::construct);
    registerMsg(msgid::GetSavedBlocksRSP, MsgEvt::GetSavedBlocksRSP::construct);
    registerMsg(msgid::DoHeartBeatREQ, MsgEvt::DoHeartBeatREQ::construct);
    registerMsg(msgid::ConfirmLeaderREQ, MsgEvt::ConfirmLeaderREQ::construct);
    registerMsg(msgid::ConfirmLeaderRSP, MsgEvt::ConfirmLeaderRSP::construct);
    registerMsg(msgid::InstructionList, MsgEvt::InstructionList::construct);
    registerMsg(msgid::TX, MsgEvt::TX::construct);
    registerMsg(msgid::TxMint, MsgEvt::TxMint::construct);

}
