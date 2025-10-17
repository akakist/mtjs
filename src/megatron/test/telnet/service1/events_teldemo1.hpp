#ifndef __________teldemo1_EventInfo__HH
#define __________teldemo1_EventInfo__HH


#include "IUtils.h"

#include "Events/Ping.h"
#include "Events/System/Net/httpEvent.h"
#include "Events/System/Net/rpcEvent.h"
#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/timerEvent.h"
#include "Events/Tools/telnetEvent.h"
inline std::set<EVENT_id> getEvents_teldemo1()
{

	std::set<EVENT_id> out;
	out.insert(rpcEventEnum::IncomingOnAcceptor);
	out.insert(rpcEventEnum::IncomingOnConnector);
	out.insert(systemEventEnum::startService);
	out.insert(telnetEventEnum::CommandEntered);
	out.insert(telnetEventEnum::RegisterCommand);
	out.insert(telnetEventEnum::Reply);
	out.insert(timerEventEnum::TickTimer);

	return out;
}

inline void regEvents_teldemo1()
{
	iUtils->registerEvent(rpcEvent::IncomingOnAcceptor::construct);
	iUtils->registerEvent(rpcEvent::IncomingOnConnector::construct);
	iUtils->registerEvent(systemEvent::startService::construct);
	iUtils->registerEvent(telnetEvent::CommandEntered::construct);
	iUtils->registerEvent(telnetEvent::RegisterCommand::construct);
	iUtils->registerEvent(telnetEvent::Reply::construct);
	iUtils->registerEvent(timerEvent::TickTimer::construct);
}
#endif
