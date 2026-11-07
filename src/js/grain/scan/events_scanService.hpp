#ifndef __________scanService_EventInfo__HH
#define __________scanService_EventInfo__HH


#include "Event/bcEvent.h"
#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/timerEvent.h"
#include "Event/bcEvent.h"
inline std::set<EVENT_id> getEvents_scanService()
{

	std::set<EVENT_id> out;
	out.insert(bcEventEnum::ClientMsg);
	out.insert(bcEventEnum::ClientMsgReply);
	out.insert(bcEventEnum::ServiceInit);
	out.insert(systemEventEnum::startService);

	return out;
}

inline void regEvents_scanService()
{
	iUtils->registerEvent(bcEvent::ClientMsg::construct);
	iUtils->registerEvent(bcEvent::ClientMsgReply::construct);
	iUtils->registerEvent(bcEvent::ServiceInit::construct);
	iUtils->registerEvent(systemEvent::startService::construct);
}
#endif
