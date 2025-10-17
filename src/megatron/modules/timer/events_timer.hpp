#ifndef __________timer_EventInfo__HH
#define __________timer_EventInfo__HH


#include "IUtils.h"

#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/timerEvent.h"
inline std::set<EVENT_id> getEvents_timer()
{

	std::set<EVENT_id> out;
	out.insert(systemEventEnum::startService);
	out.insert(timerEventEnum::ResetAlarm);
	out.insert(timerEventEnum::SetAlarm);
	out.insert(timerEventEnum::SetTimer);
	out.insert(timerEventEnum::StopAlarm);
	out.insert(timerEventEnum::StopTimer);
	out.insert(timerEventEnum::TickAlarm);
	out.insert(timerEventEnum::TickTimer);

	return out;
}

inline void regEvents_timer()
{
	iUtils->registerEvent(systemEvent::startService::construct);
	iUtils->registerEvent(timerEvent::ResetAlarm::construct);
	iUtils->registerEvent(timerEvent::SetAlarm::construct);
	iUtils->registerEvent(timerEvent::SetTimer::construct);
	iUtils->registerEvent(timerEvent::StopAlarm::construct);
	iUtils->registerEvent(timerEvent::StopTimer::construct);
	iUtils->registerEvent(timerEvent::TickAlarm::construct);
	iUtils->registerEvent(timerEvent::TickTimer::construct);
}
#endif
