#ifndef __________nodeService_EventInfo__HH
#define __________nodeService_EventInfo__HH


#include "Event/bcEvent.h"
#include "Events/System/Net/httpEvent.h"
#include "Events/System/Net/rpcEvent.h"
#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/timerEvent.h"
#include "Events/Tools/telnetEvent.h"
#include "Events/Tools/webHandlerEvent.h"
#include "Event/bcEvent.h"
#include "bcEvent.h"
#include "Events/System/Run/startServiceEvent.h"
#include "Events/Tools/telnetEvent.h"
#include "Events/Tools/webHandlerEvent.h"
#include "Events/System/Net/httpEvent.h"
#include "Events/System/timerEvent.h"
#include "Events/System/Net/httpEvent.h"
#include "Event/bcEvent.h"
inline std::set<EVENT_id> getEvents_nodeService()
{

	std::set<EVENT_id> out;
	out.insert(bcEventEnum::AddTxREQ);
	out.insert(bcEventEnum::AddTxRSP);
	out.insert(bcEventEnum::BroadcastMessage);
	out.insert(bcEventEnum::ClientMsg);
	out.insert(bcEventEnum::ClientMsgReply);
	out.insert(bcEventEnum::ForwardTxREQ);
	out.insert(bcEventEnum::GetHashedDataREQ);
	out.insert(bcEventEnum::GetHashedDataRSP);
	out.insert(bcEventEnum::GetPrevBlockREQ);
	out.insert(bcEventEnum::GetPrevBlockRSP);
	out.insert(bcEventEnum::LiteSetUplinkREQ);
	out.insert(bcEventEnum::NodeMsgREQ);
	out.insert(bcEventEnum::NodeMsgRSP);
	out.insert(bcEventEnum::NodeNotRespondedREQ);
	out.insert(bcEventEnum::OfferREQ);
	out.insert(bcEventEnum::OfferRSP);
	out.insert(bcEventEnum::ServiceInit);
	out.insert(bcEventEnum::StreamBlock);
	out.insert(bcEventEnum::syncGetGranulesREQ);
	out.insert(bcEventEnum::syncGetGranulesRSP);
	out.insert(httpEventEnum::DoListen);
	out.insert(httpEventEnum::RequestIncoming);
	out.insert(rpcEventEnum::DoListen);
	out.insert(rpcEventEnum::IncomingOnAcceptor);
	out.insert(rpcEventEnum::IncomingOnConnector);
	out.insert(systemEventEnum::startService);
	out.insert(telnetEventEnum::CommandEntered);
	out.insert(telnetEventEnum::RegisterCommand);
	out.insert(telnetEventEnum::Reply);
	out.insert(timerEventEnum::ResetAlarm);
	out.insert(timerEventEnum::SetAlarm);
	out.insert(timerEventEnum::SetTimer);
	out.insert(timerEventEnum::StopAlarm);
	out.insert(timerEventEnum::TickAlarm);
	out.insert(timerEventEnum::TickTimer);
	out.insert(webHandlerEventEnum::RequestIncoming);

	return out;
}

inline void regEvents_nodeService()
{
	iUtils->registerEvent(bcEvent::AddTxREQ::construct);
	iUtils->registerEvent(bcEvent::AddTxRSP::construct);
	iUtils->registerEvent(bcEvent::BroadcastMessage::construct);
	iUtils->registerEvent(bcEvent::ClientMsg::construct);
	iUtils->registerEvent(bcEvent::ClientMsgReply::construct);
	iUtils->registerEvent(bcEvent::ForwardTxREQ::construct);
	iUtils->registerEvent(bcEvent::GetHashedDataREQ::construct);
	iUtils->registerEvent(bcEvent::GetHashedDataRSP::construct);
	iUtils->registerEvent(bcEvent::GetPrevBlockREQ::construct);
	iUtils->registerEvent(bcEvent::GetPrevBlockRSP::construct);
	iUtils->registerEvent(bcEvent::LiteSetUplinkREQ::construct);
	iUtils->registerEvent(bcEvent::NodeMsgREQ::construct);
	iUtils->registerEvent(bcEvent::NodeMsgRSP::construct);
	iUtils->registerEvent(bcEvent::NodeNotRespondedREQ::construct);
	iUtils->registerEvent(bcEvent::OfferREQ::construct);
	iUtils->registerEvent(bcEvent::OfferRSP::construct);
	iUtils->registerEvent(bcEvent::ServiceInit::construct);
	iUtils->registerEvent(bcEvent::StreamBlock::construct);
	iUtils->registerEvent(bcEvent::syncGetGranulesREQ::construct);
	iUtils->registerEvent(bcEvent::syncGetGranulesRSP::construct);
	iUtils->registerEvent(httpEvent::DoListen::construct);
	iUtils->registerEvent(httpEvent::RequestIncoming::construct);
	iUtils->registerEvent(rpcEvent::DoListen::construct);
	iUtils->registerEvent(rpcEvent::IncomingOnAcceptor::construct);
	iUtils->registerEvent(rpcEvent::IncomingOnConnector::construct);
	iUtils->registerEvent(systemEvent::startService::construct);
	iUtils->registerEvent(telnetEvent::CommandEntered::construct);
	iUtils->registerEvent(telnetEvent::RegisterCommand::construct);
	iUtils->registerEvent(telnetEvent::Reply::construct);
	iUtils->registerEvent(timerEvent::ResetAlarm::construct);
	iUtils->registerEvent(timerEvent::SetAlarm::construct);
	iUtils->registerEvent(timerEvent::SetTimer::construct);
	iUtils->registerEvent(timerEvent::StopAlarm::construct);
	iUtils->registerEvent(timerEvent::TickAlarm::construct);
	iUtils->registerEvent(timerEvent::TickTimer::construct);
	iUtils->registerEvent(webHandlerEvent::RequestIncoming::construct);
}
#endif
