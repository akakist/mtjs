#ifndef __________ObjectProxy_EventInfo__HH
#define __________ObjectProxy_EventInfo__HH


#include "IUtils.h"

#include <Events/System/Net/rpcEvent.h>
#include <Events/System/Run/startServiceEvent.h>
inline std::set<EVENT_id> getEvents_ObjectProxy()
{

	std::set<EVENT_id> out;
	out.insert(rpcEventEnum::IncomingOnAcceptor);
	out.insert(rpcEventEnum::IncomingOnConnector);
	out.insert(systemEventEnum::startService);

	return out;
}

inline void regEvents_ObjectProxy()
{
	iUtils->registerEvent(rpcEvent::IncomingOnAcceptor::construct);
	iUtils->registerEvent(rpcEvent::IncomingOnConnector::construct);
	iUtils->registerEvent(systemEvent::startService::construct);
}
#endif
