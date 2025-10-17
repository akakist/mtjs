#ifndef __________webSocketClient_EventInfo__HH
#define __________webSocketClient_EventInfo__HH


#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/Net/rpcEvent.h"
#include "Events/System/Net/webSocketClientEvent.h"
inline std::set<EVENT_id> getEvents_webSocketClient()
{

	std::set<EVENT_id> out;
	out.insert(rpcEventEnum::IncomingOnAcceptor);
	out.insert(systemEventEnum::startService);

	return out;
}

inline void regEvents_webSocketClient()
{
	iUtils->registerEvent(rpcEvent::IncomingOnAcceptor::construct);
	iUtils->registerEvent(systemEvent::startService::construct);
}
#endif
