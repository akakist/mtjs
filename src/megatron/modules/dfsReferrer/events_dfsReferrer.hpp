#ifndef __________dfsReferrer_EventInfo__HH
#define __________dfsReferrer_EventInfo__HH


#include "IUtils.h"

#include "Events/DFS/capsEvent.h"
#include "Events/Tools/telnetEvent.h"
#include <Events/DFS/capsEvent.h>
#include <Events/DFS/referrerEvent.h>
#include <Events/System/Net/rpcEvent.h>
#include <Events/System/Run/startServiceEvent.h>
#include <Events/System/timerEvent.h>
#include <Events/Tools/telnetEvent.h>
#include <Events/Tools/webHandlerEvent.h>
inline std::set<EVENT_id> getEvents_dfsReferrer()
{

	std::set<EVENT_id> out;
	out.insert(dfsCapsEventEnum::GetReferrersREQ);
	out.insert(dfsCapsEventEnum::GetReferrersRSP);
	out.insert(dfsCapsEventEnum::RegisterMyRefferrerNodeREQ);
	out.insert(dfsReferrerEventEnum::NotifyReferrerDownlinkDisconnected);
	out.insert(dfsReferrerEventEnum::NotifyReferrerUplinkIsConnected);
	out.insert(dfsReferrerEventEnum::NotifyReferrerUplinkIsDisconnected);
	out.insert(dfsReferrerEventEnum::Ping);
	out.insert(dfsReferrerEventEnum::Pong);
	out.insert(dfsReferrerEventEnum::SubscribeNotifications);
	out.insert(dfsReferrerEventEnum::ToplinkDeliverREQ);
	out.insert(dfsReferrerEventEnum::ToplinkDeliverRSP);
	out.insert(dfsReferrerEventEnum::UpdateConfigREQ);
	out.insert(dfsReferrerEventEnum::UpdateConfigRSP);
	out.insert(rpcEventEnum::Accepted);
	out.insert(rpcEventEnum::Binded);
	out.insert(rpcEventEnum::ConnectFailed);
	out.insert(rpcEventEnum::Connected);
	out.insert(rpcEventEnum::Disaccepted);
	out.insert(rpcEventEnum::Disconnected);
	out.insert(rpcEventEnum::IncomingOnAcceptor);
	out.insert(rpcEventEnum::IncomingOnConnector);
	out.insert(rpcEventEnum::SubscribeNotifications);
	out.insert(systemEventEnum::startService);
	out.insert(telnetEventEnum::CommandEntered);
	out.insert(telnetEventEnum::RegisterCommand);
	out.insert(telnetEventEnum::Reply);
	out.insert(timerEventEnum::TickAlarm);
	out.insert(timerEventEnum::TickTimer);
	out.insert(webHandlerEventEnum::RegisterDirectory);
	out.insert(webHandlerEventEnum::RegisterHandler);
	out.insert(webHandlerEventEnum::RequestIncoming);

	return out;
}

inline void regEvents_dfsReferrer()
{
	iUtils->registerEvent(dfsCapsEvent::GetReferrersREQ::construct);
	iUtils->registerEvent(dfsCapsEvent::GetReferrersRSP::construct);
	iUtils->registerEvent(dfsCapsEvent::RegisterMyRefferrerNodeREQ::construct);
	iUtils->registerEvent(dfsReferrerEvent::NotifyReferrerDownlinkDisconnected::construct);
	iUtils->registerEvent(dfsReferrerEvent::NotifyReferrerUplinkIsConnected::construct);
	iUtils->registerEvent(dfsReferrerEvent::NotifyReferrerUplinkIsDisconnected::construct);
	iUtils->registerEvent(dfsReferrerEvent::Ping::construct);
	iUtils->registerEvent(dfsReferrerEvent::Pong::construct);
	iUtils->registerEvent(dfsReferrerEvent::SubscribeNotifications::construct);
	iUtils->registerEvent(dfsReferrerEvent::ToplinkDeliverREQ::construct);
	iUtils->registerEvent(dfsReferrerEvent::ToplinkDeliverRSP::construct);
	iUtils->registerEvent(dfsReferrerEvent::UpdateConfigREQ::construct);
	iUtils->registerEvent(dfsReferrerEvent::UpdateConfigRSP::construct);
	iUtils->registerEvent(rpcEvent::Accepted::construct);
	iUtils->registerEvent(rpcEvent::Binded::construct);
	iUtils->registerEvent(rpcEvent::ConnectFailed::construct);
	iUtils->registerEvent(rpcEvent::Connected::construct);
	iUtils->registerEvent(rpcEvent::Disaccepted::construct);
	iUtils->registerEvent(rpcEvent::Disconnected::construct);
	iUtils->registerEvent(rpcEvent::IncomingOnAcceptor::construct);
	iUtils->registerEvent(rpcEvent::IncomingOnConnector::construct);
	iUtils->registerEvent(rpcEvent::SubscribeNotifications::construct);
	iUtils->registerEvent(systemEvent::startService::construct);
	iUtils->registerEvent(telnetEvent::CommandEntered::construct);
	iUtils->registerEvent(telnetEvent::RegisterCommand::construct);
	iUtils->registerEvent(telnetEvent::Reply::construct);
	iUtils->registerEvent(timerEvent::TickAlarm::construct);
	iUtils->registerEvent(timerEvent::TickTimer::construct);
	iUtils->registerEvent(webHandlerEvent::RegisterDirectory::construct);
	iUtils->registerEvent(webHandlerEvent::RegisterHandler::construct);
	iUtils->registerEvent(webHandlerEvent::RequestIncoming::construct);
}
#endif
