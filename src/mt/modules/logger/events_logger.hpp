#ifndef __________logger_EventInfo__HH
#define __________logger_EventInfo__HH


#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/Net/rpcEvent.h"
#include "Events/Tools/loggerEvent.h"
inline std::set<EVENT_id> getEvents_logger()
{

	std::set<EVENT_id> out;
	out.insert(loggerEventEnum::LogMessage);
	out.insert(rpcEventEnum::IncomingOnAcceptor);
	out.insert(rpcEventEnum::IncomingOnConnector);
	out.insert(systemEventEnum::startService);

	return out;
}

inline void regEvents_logger()
{
	iUtils->registerEvent(loggerEvent::LogMessage::construct);
	iUtils->registerEvent(rpcEvent::IncomingOnAcceptor::construct);
	iUtils->registerEvent(rpcEvent::IncomingOnConnector::construct);
	iUtils->registerEvent(systemEvent::startService::construct);
}
#endif
