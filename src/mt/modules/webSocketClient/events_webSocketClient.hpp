#ifndef __________webSocketClient_EventInfo__HH
#define __________webSocketClient_EventInfo__HH


#include "Events/System/Net/socketEvent.h"
#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/Net/rpcEvent.h"
#include "Events/System/Net/webSocketClientEvent.h"
#include "Events/System/Net/socketEvent.h"
inline std::set<EVENT_id> getEvents_webSocketClient()
{

	std::set<EVENT_id> out;
	out.insert(rpcEventEnum::IncomingOnAcceptor);
	out.insert(socketEventEnum::AddToConnectTCP);
	out.insert(socketEventEnum::ConnectFailed);
	out.insert(socketEventEnum::Connected);
	out.insert(socketEventEnum::Disconnected);
	out.insert(socketEventEnum::NotifyOutBufferEmpty);
	out.insert(socketEventEnum::StreamRead);
	out.insert(systemEventEnum::startService);
	out.insert(webSocketClientEventEnum::Connect);
	out.insert(webSocketClientEventEnum::ConnectFailed);
	out.insert(webSocketClientEventEnum::Connected);
	out.insert(webSocketClientEventEnum::Disconnected);
	out.insert(webSocketClientEventEnum::Received);
	out.insert(webSocketClientEventEnum::Send);

	return out;
}

inline void regEvents_webSocketClient()
{
	iUtils->registerEvent(rpcEvent::IncomingOnAcceptor::construct);
	iUtils->registerEvent(socketEvent::AddToConnectTCP::construct);
	iUtils->registerEvent(socketEvent::ConnectFailed::construct);
	iUtils->registerEvent(socketEvent::Connected::construct);
	iUtils->registerEvent(socketEvent::Disconnected::construct);
	iUtils->registerEvent(socketEvent::NotifyOutBufferEmpty::construct);
	iUtils->registerEvent(socketEvent::StreamRead::construct);
	iUtils->registerEvent(systemEvent::startService::construct);
	iUtils->registerEvent(webSocketClientEvent::Connect::construct);
	iUtils->registerEvent(webSocketClientEvent::ConnectFailed::construct);
	iUtils->registerEvent(webSocketClientEvent::Connected::construct);
	iUtils->registerEvent(webSocketClientEvent::Disconnected::construct);
	iUtils->registerEvent(webSocketClientEvent::Received::construct);
	iUtils->registerEvent(webSocketClientEvent::Send::construct);
}
#endif
