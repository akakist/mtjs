#include "msgFactory.h"
#include "msg.h"
#include "md/md_HeartBeatREQ.h"
#include "md/md_HeartBeatRSP.h"
#include "md/md_GetTransactionREQ.h"
#include "md/md_GetTransactionRSP.h"
#include "md/md_ValidateBlockREQ.h"
#include "md/md_ValidateBlockRSP.h"
#include "md/md_BlockAcceptedREQ.h"

#include "md/md_ConfirmLeaderREQ.h"
#include "md/md_ConfirmLeaderRSP.h"
#include "md/md_TX.h"
#include "md/md_GetUserNonceREQ.h"
#include "md/md_GetUserNonceRSP.h"
#include "md/md_LcEnvelopeREQ.h"
#include "md/md_BlockDiffValidateREQ.h"
#include "md/md_BlockDiffValidateRSP.h"
#include "md/md_BlockAccepted2REQ.h"
#include "md/md_BlockDBStore.h"


#include "md/md_GetNodeInfoREQ.h"
#include "md/md_GetNodeInfoRSP.h"
#include "md/md_GetValuesREQ.h"
#include "md/md_GetValuesRSP.h"
#include "md/md_GetContractDataREQ.h"
#include "md/md_GetContractDataRSP.h"
#include "md/md_DelayNotificationREQ.h"
#include "md/md_GetContractREQ.h"
#include "md/md_GetContractRSP.h"
#include "md/md_GetStateREQ.h"
#include "md/md_GetStateRSP.h"
#include "md/md_GetAddressInfoREQ.h"
#include "md/md_GetAddressInfoRSP.h"
#include "md/md_CallViewREQ.h"
#include "md/md_CallViewRSP.h"
#include "md/md_GetGranuleREQ.h"
#include "md/md_GetGranuleRSP.h"
#include "md/md_ScanREQ.h"
#include "md/md_ScanRSP.h"


MsgFactory::MsgFactory()
{
    registerMsg(msgid::HeartBeatREQ, MsgData::HeartBeatREQ::construct);
    registerMsg(msgid::HeartBeatRSP, MsgData::HeartBeatRSP::construct);
    registerMsg(msgid::GetTransactionREQ, MsgData::GetTransactionREQ::construct);
    registerMsg(msgid::GetTransactionRSP, MsgData::GetTransactionRSP::construct);
    registerMsg(msgid::ValidateBlockREQ, MsgData::ValidateBlockREQ::construct);
    registerMsg(msgid::ValidateBlockRSP, MsgData::ValidateBlockRSP::construct);
    registerMsg(msgid::BlockAcceptedREQ, MsgData::BlockAcceptedREQ::construct);
    registerMsg(msgid::BlockAccepted2REQ, MsgData::BlockAccepted2REQ::construct);
    registerMsg(msgid::ConfirmLeaderREQ, MsgData::ConfirmLeaderREQ::construct);
    registerMsg(msgid::ConfirmLeaderRSP, MsgData::ConfirmLeaderRSP::construct);
    registerMsg(msgid::TX, MsgData::TX::construct);
    registerMsg(msgid::GetUserNonceREQ, MsgData::GetUserNonceREQ::construct);
    registerMsg(msgid::GetUserNonceRSP, MsgData::GetUserNonceRSP::construct);
    registerMsg(msgid::LcEnvelopeREQ, MsgData::LcEnvelopeREQ::construct);
    registerMsg(msgid::DelayNotificationREQ, MsgData::DelayNotificationREQ::construct);

    registerMsg(msgid::GetNodeInfoREQ, MsgData::GetNodeInfoREQ::construct);
    registerMsg(msgid::GetNodeInfoRSP, MsgData::GetNodeInfoRSP::construct);
    registerMsg(msgid::GetValuesREQ, MsgData::GetValuesREQ::construct);
    registerMsg(msgid::GetValuesRSP, MsgData::GetValuesRSP::construct);
    registerMsg(msgid::GetContractDataREQ, MsgData::GetContractDataREQ::construct);
    registerMsg(msgid::GetContractDataRSP, MsgData::GetContractDataRSP::construct);
    registerMsg(msgid::BlockDiffValidateREQ, MsgData::BlockDiffValidateREQ::construct);
    registerMsg(msgid::BlockDiffValidateRSP, MsgData::BlockDiffValidateRSP::construct);
    registerMsg(msgid::BlockDBStore, MsgData::BlockDBStore::construct);
    registerMsg(msgid::GetContractREQ, MsgData::GetContractREQ::construct);
    registerMsg(msgid::GetContractRSP, MsgData::GetContractRSP::construct);

    // в MsgFactory::MsgFactory():
    registerMsg(msgid::GetStateREQ, MsgData::GetStateREQ::construct);
    registerMsg(msgid::GetStateRSP, MsgData::GetStateRSP::construct);
    registerMsg(msgid::GetAddressInfoREQ, MsgData::GetAddressInfoREQ::construct);
    registerMsg(msgid::GetAddressInfoRSP, MsgData::GetAddressInfoRSP::construct);
    registerMsg(msgid::CallViewREQ, MsgData::CallViewREQ::construct);
    registerMsg(msgid::CallViewRSP, MsgData::CallViewRSP::construct);
    registerMsg(msgid::GetGranuleREQ, MsgData::GetGranuleREQ::construct);
    registerMsg(msgid::GetGranuleRSP, MsgData::GetGranuleRSP::construct);    
    registerMsg(msgid::ScanREQ, MsgData::ScanREQ::construct);
    registerMsg(msgid::ScanRSP, MsgData::ScanRSP::construct);
}
