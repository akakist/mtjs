#include "msgFactory.h"
#include "msg.h"
#include "md/md_HeartBeatREQ.h"
#include "md/md_HeartBeatRSP.h" 
#include "md/md_GetTransactionREQ.h"
#include "md/md_GetTransactionRSP.h"
#include "md/md_ValidateBlockREQ.h"
#include "md/md_ValidateBlockRSP.h"
#include "md/md_BlockAcceptedREQ.h"
#include "md/md_BlockAcceptedRSP.h"
#include "md/md_GetSavedBlocksREQ.h"
#include "md/md_GetSavedBlocksRSP.h"
#include "md/md_DoHeartBeatREQ.h"
#include "md/md_ConfirmLeaderREQ.h"
#include "md/md_ConfirmLeaderRSP.h"
#include "md/md_InstructionList.h"
#include "md/md_TX.h"
#include "md/md_TxMint.h"

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
