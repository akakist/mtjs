#ifndef __________mtjs_EventInfo__HH
#define __________mtjs_EventInfo__HH


#include "common/mtjsEvent.h"
#include <Events/System/timerEvent.h>
#include "common/mtjsEvent.h"
#include "Events/Tools/webHandlerEvent.h"
#include "Events/Tools/webHandlerEvent.h"
#include <Events/System/Run/startServiceEvent.h>
#include "Events/System/Net/httpEvent.h"
#include "Events/System/timerEvent.h"
#include "Events/System/Net/socketEvent.h"
#include "Events/System/Net/rpcEvent.h"
#include "common/mtjsEvent.h"
#include "Events/Tools/webHandlerEvent.h"
#include "Events/Tools/telnetEvent.h"
inline std::set<EVENT_id> getEvents_mtjs()
{

	std::set<EVENT_id> out;
	out.insert(httpEventEnum::RequestChunkReceived);
	out.insert(httpEventEnum::RequestChunkingCompleted);
	out.insert(httpEventEnum::RequestIncoming);
	out.insert(httpEventEnum::RequestStartChunking);
	out.insert(httpEventEnum::WSDisaccepted);
	out.insert(httpEventEnum::WSDisconnected);
	out.insert(httpEventEnum::WSTextMessage);
	out.insert(mtjsEventEnum::AsyncExecuted);
	out.insert(mtjsEventEnum::EmitterData);
	out.insert(mtjsEventEnum::Eval);
	out.insert(mtjsEventEnum::mtjsRpcREQ);
	out.insert(mtjsEventEnum::mtjsRpcRSP);
	out.insert(rpcEventEnum::IncomingOnAcceptor);
	out.insert(rpcEventEnum::IncomingOnConnector);
	out.insert(socketEventEnum::Connected);
	out.insert(socketEventEnum::Disconnected);
	out.insert(socketEventEnum::NotifyOutBufferEmpty);
	out.insert(socketEventEnum::StreamRead);
	out.insert(systemEventEnum::startService);
	out.insert(telnetEventEnum::CommandEntered);
	out.insert(timerEventEnum::SetTimer);
	out.insert(timerEventEnum::TickAlarm);
	out.insert(timerEventEnum::TickTimer);
	out.insert(webHandlerEventEnum::RegisterDirectory);
	out.insert(webHandlerEventEnum::RegisterHandler);
	out.insert(webHandlerEventEnum::RequestIncoming);

	return out;
}

inline void regEvents_mtjs()
{
	iUtils->registerEvent(httpEvent::RequestChunkReceived::construct);
	iUtils->registerEvent(httpEvent::RequestChunkingCompleted::construct);
	iUtils->registerEvent(httpEvent::RequestIncoming::construct);
	iUtils->registerEvent(httpEvent::RequestStartChunking::construct);
	iUtils->registerEvent(httpEvent::WSDisaccepted::construct);
	iUtils->registerEvent(httpEvent::WSDisconnected::construct);
	iUtils->registerEvent(httpEvent::WSTextMessage::construct);
	iUtils->registerEvent(mtjsEvent::AsyncExecuted::construct);
	iUtils->registerEvent(mtjsEvent::EmitterData::construct);
	iUtils->registerEvent(mtjsEvent::Eval::construct);
	iUtils->registerEvent(mtjsEvent::mtjsRpcREQ::construct);
	iUtils->registerEvent(mtjsEvent::mtjsRpcRSP::construct);
	iUtils->registerEvent(rpcEvent::IncomingOnAcceptor::construct);
	iUtils->registerEvent(rpcEvent::IncomingOnConnector::construct);
	iUtils->registerEvent(socketEvent::Connected::construct);
	iUtils->registerEvent(socketEvent::Disconnected::construct);
	iUtils->registerEvent(socketEvent::NotifyOutBufferEmpty::construct);
	iUtils->registerEvent(socketEvent::StreamRead::construct);
	iUtils->registerEvent(systemEvent::startService::construct);
	iUtils->registerEvent(telnetEvent::CommandEntered::construct);
	iUtils->registerEvent(timerEvent::SetTimer::construct);
	iUtils->registerEvent(timerEvent::TickAlarm::construct);
	iUtils->registerEvent(timerEvent::TickTimer::construct);
	iUtils->registerEvent(webHandlerEvent::RegisterDirectory::construct);
	iUtils->registerEvent(webHandlerEvent::RegisterHandler::construct);
	iUtils->registerEvent(webHandlerEvent::RequestIncoming::construct);
}
#endif
