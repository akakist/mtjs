#ifndef __________webHandler_EventInfo__HH
#define __________webHandler_EventInfo__HH


#include <Events/System/Net/httpEvent.h>
#include "Events/Tools/webHandlerEvent.h"
#include <Events/System/Run/startServiceEvent.h>
#include <Events/System/Net/httpEvent.h>
#include "Events/Tools/webHandlerEvent.h"
#include <Events/System/Net/httpEvent.h>
inline std::set<EVENT_id> getEvents_webHandler()
{

	std::set<EVENT_id> out;
	out.insert(httpEventEnum::DoListen);
	out.insert(httpEventEnum::GetBindPortsRSP);
	out.insert(httpEventEnum::RequestIncoming);
	out.insert(systemEventEnum::startService);
	out.insert(webHandlerEventEnum::RegisterDirectory);
	out.insert(webHandlerEventEnum::RegisterHandler);
	out.insert(webHandlerEventEnum::RequestIncoming);

	return out;
}

inline void regEvents_webHandler()
{
	iUtils->registerEvent(httpEvent::DoListen::construct);
	iUtils->registerEvent(httpEvent::GetBindPortsRSP::construct);
	iUtils->registerEvent(httpEvent::RequestIncoming::construct);
	iUtils->registerEvent(systemEvent::startService::construct);
	iUtils->registerEvent(webHandlerEvent::RegisterDirectory::construct);
	iUtils->registerEvent(webHandlerEvent::RegisterHandler::construct);
	iUtils->registerEvent(webHandlerEvent::RequestIncoming::construct);
}
#endif
