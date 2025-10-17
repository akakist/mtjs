#ifndef __________prodtestServerService_EventInfo__HH
#define __________prodtestServerService_EventInfo__HH


#include "IUtils.h"

#include "Events/Ping.h"
#include "Events/System/Net/rpcEvent.h"
#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/timerEvent.h"
inline std::set<EVENT_id> getEvents_prodtestServerService()
{

	std::set<EVENT_id> out;
	out.insert(prodtestEventEnum::AddTaskREQ);
	out.insert(prodtestEventEnum::AddTaskRSP);
	out.insert(rpcEventEnum::IncomingOnAcceptor);
	out.insert(systemEventEnum::startService);
	out.insert(timerEventEnum::TickTimer);

	return out;
}

inline void regEvents_prodtestServerService()
{
	iUtils->registerEvent(prodtestEvent::AddTaskREQ::construct);
	iUtils->registerEvent(prodtestEvent::AddTaskRSP::construct);
	iUtils->registerEvent(rpcEvent::IncomingOnAcceptor::construct);
	iUtils->registerEvent(systemEvent::startService::construct);
	iUtils->registerEvent(timerEvent::TickTimer::construct);
}
#endif
