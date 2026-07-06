#include "msgFactory.h"
#include "msg.h"
#include "md/md_HeartBeatREQ.h"
#include "md/md_HeartBeatRSP.h"
#include "md/md_GetTransactionREQ.h"
#include "md/md_GetTransactionRSP.h"
#include "md/md_ValidateBlockREQ.h"
#include "md/md_ValidateBlockRSP.h"
#include "md/md_BlockAcceptedREQ.h"
#include "md/md_GetSavedBlocksREQ.h"
#include "md/md_GetSavedBlocksRSP.h"
#include "md/md_DoHeartBeatREQ.h"
#include "md/md_ConfirmLeaderREQ.h"
#include "md/md_ConfirmLeaderRSP.h"
#include "md/md_TX.h"
#include "md/md_GetUserNonceREQ.h"
#include "md/md_GetUserNonceRSP.h"
#include "md/md_LcEnvelopeREQ.h"
#include "md/md_DoYouHaveBlockREQ.h"
#include "md/md_DoYouHaveBlockRSP.h"
#include "md/md_LcREQ.h"
#include "md/md_LcRSP.h"


MsgFactory::MsgFactory()
{
    registerMsg(msgid::HeartBeatREQ, MsgData::HeartBeatREQ::construct);
    registerMsg(msgid::HeartBeatRSP, MsgData::HeartBeatRSP::construct);
    registerMsg(msgid::GetTransactionREQ, MsgData::GetTransactionREQ::construct);
    registerMsg(msgid::GetTransactionRSP, MsgData::GetTransactionRSP::construct);
    registerMsg(msgid::ValidateBlockREQ, MsgData::ValidateBlockREQ::construct);
    registerMsg(msgid::ValidateBlockRSP, MsgData::ValidateBlockRSP::construct);
    registerMsg(msgid::BlockAcceptedREQ, MsgData::BlockAcceptedREQ::construct);
    registerMsg(msgid::GetSavedBlocksREQ, MsgData::GetSavedBlocksREQ::construct);
    registerMsg(msgid::GetSavedBlocksRSP, MsgData::GetSavedBlocksRSP::construct);
    registerMsg(msgid::ConfirmLeaderREQ, MsgData::ConfirmLeaderREQ::construct);
    registerMsg(msgid::ConfirmLeaderRSP, MsgData::ConfirmLeaderRSP::construct);
    registerMsg(msgid::TX, MsgData::TX::construct);
    registerMsg(msgid::GetUserNonceREQ, MsgData::GetUserNonceREQ::construct);
    registerMsg(msgid::GetUserNonceRSP, MsgData::GetUserNonceRSP::construct);
    registerMsg(msgid::LcEnvelopeREQ, MsgData::LcEnvelopeREQ::construct);
    registerMsg(msgid::LcREQ, MsgData::LcREQ::construct);
    registerMsg(msgid::LcRSP, MsgData::LcRSP::construct);
    registerMsg(msgid::DoYouHaveBlockREQ, MsgData::DoYouHaveBlockREQ::construct);
    registerMsg(msgid::DoYouHaveBlockRSP, MsgData::DoYouHaveBlockRSP::construct);

}
