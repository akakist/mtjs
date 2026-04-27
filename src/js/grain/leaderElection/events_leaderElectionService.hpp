#ifndef __________leaderElectionService_EventInfo__HH
#define __________leaderElectionService_EventInfo__HH


#include "Events/System/Net/rpcEvent.h"
#include "Events/System/timerEvent.h"
#include "Event/bcEvent.h"
#include "Events/System/Run/startServiceEvent.h"
#include "Events/Tools/telnetEvent.h"
#include "Events/Tools/webHandlerEvent.h"
#include "Events/System/Net/httpEvent.h"
#include "Events/System/timerEvent.h"
#include "Events/System/Net/httpEvent.h"
#include "Event/bcEvent.h"
inline std::set<EVENT_id> getEvents_leaderElectionService()
{

	std::set<EVENT_id> out;
	out.insert(bcEventEnum::AddTx);
	out.insert(bcEventEnum::ClientMsg);
	out.insert(bcEventEnum::ClientTxSubscribeREQ);
	out.insert(bcEventEnum::GetTransactions);
	out.insert(bcEventEnum::InvalidateRoot);
	out.insert(bcEventEnum::ServiceInit);
	out.insert(bcEventEnum::StreamBlock);
	out.insert(rpcEventEnum::IncomingOnAcceptor);
	out.insert(rpcEventEnum::IncomingOnConnector);
	out.insert(systemEventEnum::startService);
	out.insert(timerEventEnum::TickAlarm);
	out.insert(timerEventEnum::TickTimer);

	return out;
}

inline void regEvents_leaderElectionService()
{
	iUtils->registerEvent(bcEvent::AddTx::construct);
	iUtils->registerEvent(bcEvent::ClientMsg::construct);
	iUtils->registerEvent(bcEvent::ClientTxSubscribeREQ::construct);
	iUtils->registerEvent(bcEvent::GetTransactions::construct);
	iUtils->registerEvent(bcEvent::InvalidateRoot::construct);
	iUtils->registerEvent(bcEvent::ServiceInit::construct);
	iUtils->registerEvent(bcEvent::StreamBlock::construct);
	iUtils->registerEvent(rpcEvent::IncomingOnAcceptor::construct);
	iUtils->registerEvent(rpcEvent::IncomingOnConnector::construct);
	iUtils->registerEvent(systemEvent::startService::construct);
	iUtils->registerEvent(timerEvent::TickAlarm::construct);
	iUtils->registerEvent(timerEvent::TickTimer::construct);
}
#endif
