#ifndef __________grainWriterService_EventInfo__HH
#define __________grainWriterService_EventInfo__HH


#include "Events/System/Net/rpcEvent.h"
#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/timerEvent.h"
#include "Event/bcEvent.h"
#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/timerEvent.h"
#include "Event/bcEvent.h"
inline std::set<EVENT_id> getEvents_grainWriterService()
{

	std::set<EVENT_id> out;
	out.insert(bcEventEnum::ClientMsg);
	out.insert(bcEventEnum::InvalidateRoot);
	out.insert(bcEventEnum::ServiceInit);
	out.insert(bcEventEnum::WriteGranules);
	out.insert(rpcEventEnum::IncomingOnAcceptor);
	out.insert(rpcEventEnum::IncomingOnConnector);
	out.insert(systemEventEnum::startService);
	out.insert(timerEventEnum::SetAlarm);
	out.insert(timerEventEnum::SetTimer);
	out.insert(timerEventEnum::TickAlarm);
	out.insert(timerEventEnum::TickTimer);

	return out;
}

inline void regEvents_grainWriterService()
{
	iUtils->registerEvent(bcEvent::ClientMsg::construct);
	iUtils->registerEvent(bcEvent::InvalidateRoot::construct);
	iUtils->registerEvent(bcEvent::ServiceInit::construct);
	iUtils->registerEvent(bcEvent::WriteGranules::construct);
	iUtils->registerEvent(rpcEvent::IncomingOnAcceptor::construct);
	iUtils->registerEvent(rpcEvent::IncomingOnConnector::construct);
	iUtils->registerEvent(systemEvent::startService::construct);
	iUtils->registerEvent(timerEvent::SetAlarm::construct);
	iUtils->registerEvent(timerEvent::SetTimer::construct);
	iUtils->registerEvent(timerEvent::TickAlarm::construct);
	iUtils->registerEvent(timerEvent::TickTimer::construct);
}
#endif
