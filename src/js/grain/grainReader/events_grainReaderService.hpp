#ifndef __________grainReaderService_EventInfo__HH
#define __________grainReaderService_EventInfo__HH


#include "Events/System/Net/rpcEvent.h"
#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/timerEvent.h"
#include "Event/bcEvent.h"
#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/Net/httpEvent.h"
#include "Events/System/timerEvent.h"
#include "Event/bcEvent.h"
inline std::set<EVENT_id> getEvents_grainReaderService()
{

	std::set<EVENT_id> out;
	out.insert(bcEventEnum::ClientMsg);
	out.insert(bcEventEnum::ClientMsgReply);
	out.insert(bcEventEnum::InvalidateRoot);
	out.insert(bcEventEnum::ServiceInit);
	out.insert(bcEventEnum::StaleGranules);
	out.insert(httpEventEnum::DoListen);
	out.insert(httpEventEnum::RequestIncoming);
	out.insert(rpcEventEnum::IncomingOnAcceptor);
	out.insert(rpcEventEnum::IncomingOnConnector);
	out.insert(systemEventEnum::startService);
	out.insert(timerEventEnum::SetTimer);
	out.insert(timerEventEnum::TickAlarm);
	out.insert(timerEventEnum::TickTimer);

	return out;
}

inline void regEvents_grainReaderService()
{
	iUtils->registerEvent(bcEvent::ClientMsg::construct);
	iUtils->registerEvent(bcEvent::ClientMsgReply::construct);
	iUtils->registerEvent(bcEvent::InvalidateRoot::construct);
	iUtils->registerEvent(bcEvent::ServiceInit::construct);
	iUtils->registerEvent(bcEvent::StaleGranules::construct);
	iUtils->registerEvent(httpEvent::DoListen::construct);
	iUtils->registerEvent(httpEvent::RequestIncoming::construct);
	iUtils->registerEvent(rpcEvent::IncomingOnAcceptor::construct);
	iUtils->registerEvent(rpcEvent::IncomingOnConnector::construct);
	iUtils->registerEvent(systemEvent::startService::construct);
	iUtils->registerEvent(timerEvent::SetTimer::construct);
	iUtils->registerEvent(timerEvent::TickAlarm::construct);
	iUtils->registerEvent(timerEvent::TickTimer::construct);
}
#endif
