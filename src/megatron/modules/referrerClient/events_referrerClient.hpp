#ifndef __________referrerClient_EventInfo__HH
#define __________referrerClient_EventInfo__HH


#include "IUtils.h"

#include "Events/DFS/capsEvent.h"
#include "Events/DFS/referrerEvent.h"
#include "Events/System/Net/rpcEvent.h"
#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/timerEvent.h"
#include "Events/Tools/telnetEvent.h"
#include "Events/Tools/webHandlerEvent.h"
inline std::set<EVENT_id> getEvents_referrerClient()
{

	std::set<EVENT_id> out;
	out.insert(dfsCapsEventEnum::GetReferrersREQ);
	out.insert(dfsCapsEventEnum::GetReferrersRSP);
	out.insert(dfsReferrerEventEnum::InitClient);
	out.insert(dfsReferrerEventEnum::NotifyReferrerUplinkIsConnected);
	out.insert(dfsReferrerEventEnum::NotifyReferrerUplinkIsDisconnected);
	out.insert(dfsReferrerEventEnum::Ping);
	out.insert(dfsReferrerEventEnum::Pong);
	out.insert(dfsReferrerEventEnum::SubscribeNotifications);
	out.insert(dfsReferrerEventEnum::ToplinkDeliverREQ);
	out.insert(dfsReferrerEventEnum::ToplinkDeliverRSP);
	out.insert(dfsReferrerEventEnum::UpdateConfigRSP);
	out.insert(rpcEventEnum::ConnectFailed);
	out.insert(rpcEventEnum::Connected);
	out.insert(rpcEventEnum::Disconnected);
	out.insert(rpcEventEnum::IncomingOnConnector);
	out.insert(rpcEventEnum::SubscribeNotifications);
	out.insert(systemEventEnum::startService);
	out.insert(telnetEventEnum::CommandEntered);
	out.insert(telnetEventEnum::RegisterCommand);
	out.insert(timerEventEnum::TickAlarm);
	out.insert(timerEventEnum::TickTimer);
	out.insert(webHandlerEventEnum::RegisterDirectory);
	out.insert(webHandlerEventEnum::RegisterHandler);
	out.insert(webHandlerEventEnum::RequestIncoming);

	return out;
}

inline void regEvents_referrerClient()
{
	iUtils->registerEvent(dfsCapsEvent::GetReferrersREQ::construct);
	iUtils->registerEvent(dfsCapsEvent::GetReferrersRSP::construct);
	iUtils->registerEvent(dfsReferrerEvent::InitClient::construct);
	iUtils->registerEvent(dfsReferrerEvent::NotifyReferrerUplinkIsConnected::construct);
	iUtils->registerEvent(dfsReferrerEvent::NotifyReferrerUplinkIsDisconnected::construct);
	iUtils->registerEvent(dfsReferrerEvent::Ping::construct);
	iUtils->registerEvent(dfsReferrerEvent::Pong::construct);
	iUtils->registerEvent(dfsReferrerEvent::SubscribeNotifications::construct);
	iUtils->registerEvent(dfsReferrerEvent::ToplinkDeliverREQ::construct);
	iUtils->registerEvent(dfsReferrerEvent::ToplinkDeliverRSP::construct);
	iUtils->registerEvent(dfsReferrerEvent::UpdateConfigRSP::construct);
	iUtils->registerEvent(rpcEvent::ConnectFailed::construct);
	iUtils->registerEvent(rpcEvent::Connected::construct);
	iUtils->registerEvent(rpcEvent::Disconnected::construct);
	iUtils->registerEvent(rpcEvent::IncomingOnConnector::construct);
	iUtils->registerEvent(rpcEvent::SubscribeNotifications::construct);
	iUtils->registerEvent(systemEvent::startService::construct);
	iUtils->registerEvent(telnetEvent::CommandEntered::construct);
	iUtils->registerEvent(telnetEvent::RegisterCommand::construct);
	iUtils->registerEvent(timerEvent::TickAlarm::construct);
	iUtils->registerEvent(timerEvent::TickTimer::construct);
	iUtils->registerEvent(webHandlerEvent::RegisterDirectory::construct);
	iUtils->registerEvent(webHandlerEvent::RegisterHandler::construct);
	iUtils->registerEvent(webHandlerEvent::RequestIncoming::construct);
}
#endif
