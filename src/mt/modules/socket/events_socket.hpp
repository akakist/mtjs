#ifndef __________socket_EventInfo__HH
#define __________socket_EventInfo__HH


#include <Events/System/Net/socketEvent.h>
#include "Events/Tools/webHandlerEvent.h"
#include <Events/System/Net/socketEvent.h>
#include <Events/System/Run/startServiceEvent.h>
#include <Events/System/timerEvent.h>
#include <Events/Tools/webHandlerEvent.h>
inline std::set<EVENT_id> getEvents_socket()
{

	std::set<EVENT_id> out;
	out.insert(socketEventEnum::Accepted);
	out.insert(socketEventEnum::AddToConnectTCP);
	out.insert(socketEventEnum::AddToListenTCP);
	out.insert(socketEventEnum::ConnectFailed);
	out.insert(socketEventEnum::Connected);
	out.insert(socketEventEnum::Disaccepted);
	out.insert(socketEventEnum::Disconnected);
	out.insert(socketEventEnum::NotifyBindAddress);
	out.insert(socketEventEnum::NotifyOutBufferEmpty);
	out.insert(socketEventEnum::StreamRead);
	out.insert(systemEventEnum::startService);
	out.insert(webHandlerEventEnum::RequestIncoming);

	return out;
}

inline void regEvents_socket()
{
	iUtils->registerEvent(socketEvent::Accepted::construct);
	iUtils->registerEvent(socketEvent::AddToConnectTCP::construct);
	iUtils->registerEvent(socketEvent::AddToListenTCP::construct);
	iUtils->registerEvent(socketEvent::ConnectFailed::construct);
	iUtils->registerEvent(socketEvent::Connected::construct);
	iUtils->registerEvent(socketEvent::Disaccepted::construct);
	iUtils->registerEvent(socketEvent::Disconnected::construct);
	iUtils->registerEvent(socketEvent::NotifyBindAddress::construct);
	iUtils->registerEvent(socketEvent::NotifyOutBufferEmpty::construct);
	iUtils->registerEvent(socketEvent::StreamRead::construct);
	iUtils->registerEvent(systemEvent::startService::construct);
	iUtils->registerEvent(webHandlerEvent::RequestIncoming::construct);
}
#endif
