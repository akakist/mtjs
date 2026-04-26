#ifndef __________nodeService_EventInfo__HH
#define __________nodeService_EventInfo__HH


#include "Events/System/Net/rpcEvent.h"
#include "Events/System/timerEvent.h"
#include "Event/bcEvent.h"
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
	out.insert(bcEventEnum::BroadcastMessage);
	out.insert(bcEventEnum::ClientMsg);
	out.insert(bcEventEnum::ClientMsgReply);
	out.insert(bcEventEnum::ClientTxSubscribeREQ);
	out.insert(bcEventEnum::ClientTxSubscribeRSP);
	out.insert(bcEventEnum::GetTransactions);
	out.insert(bcEventEnum::InvalidateRoot);
	out.insert(bcEventEnum::Msg);
	out.insert(bcEventEnum::MsgReply);
	out.insert(bcEventEnum::ServiceInit);
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
	out.insert(timerEventEnum::StopAlarm);
	out.insert(timerEventEnum::TickAlarm);
	out.insert(timerEventEnum::TickTimer);
	out.insert(webHandlerEventEnum::RequestIncoming);

	return out;
}

inline void regEvents_nodeService()
{
	iUtils->registerEvent(bcEvent::BroadcastMessage::construct);
	iUtils->registerEvent(bcEvent::ClientMsg::construct);
	iUtils->registerEvent(bcEvent::ClientMsgReply::construct);
	iUtils->registerEvent(bcEvent::ClientTxSubscribeREQ::construct);
	iUtils->registerEvent(bcEvent::ClientTxSubscribeRSP::construct);
	iUtils->registerEvent(bcEvent::GetTransactions::construct);
	iUtils->registerEvent(bcEvent::InvalidateRoot::construct);
	iUtils->registerEvent(bcEvent::Msg::construct);
	iUtils->registerEvent(bcEvent::MsgReply::construct);
	iUtils->registerEvent(bcEvent::ServiceInit::construct);
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
	iUtils->registerEvent(timerEvent::StopAlarm::construct);
	iUtils->registerEvent(timerEvent::TickAlarm::construct);
	iUtils->registerEvent(timerEvent::TickTimer::construct);
	iUtils->registerEvent(webHandlerEvent::RequestIncoming::construct);
}
#endif
