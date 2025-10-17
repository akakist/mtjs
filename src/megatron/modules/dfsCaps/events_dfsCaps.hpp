#ifndef __________dfsCaps_EventInfo__HH
#define __________dfsCaps_EventInfo__HH


#include "IUtils.h"

#include "Events/DFS/capsEvent.h"
#include "Events/DFS/referrerEvent.h"
#include "Events/System/Net/rpcEvent.h"
#include "Events/System/Run/startServiceEvent.h"
#include "Events/System/timerEvent.h"
#include "Events/Tools/webHandlerEvent.h"
inline std::set<EVENT_id> getEvents_dfsCaps()
{

	std::set<EVENT_id> out;
	out.insert(dfsCapsEventEnum::GetCloudRootsREQ);
	out.insert(dfsCapsEventEnum::GetCloudRootsRSP);
	out.insert(dfsCapsEventEnum::GetReferrersREQ);
	out.insert(dfsCapsEventEnum::GetReferrersRSP);
	out.insert(dfsCapsEventEnum::RegisterCloudRoot);
	out.insert(dfsCapsEventEnum::RegisterMyRefferrerNodeREQ);
	out.insert(dfsReferrerEventEnum::ToplinkDeliverREQ);
	out.insert(dfsReferrerEventEnum::ToplinkDeliverRSP);
	out.insert(rpcEventEnum::IncomingOnAcceptor);
	out.insert(rpcEventEnum::IncomingOnConnector);
	out.insert(systemEventEnum::startService);
	out.insert(timerEventEnum::TickAlarm);

	return out;
}

inline void regEvents_dfsCaps()
{
	iUtils->registerEvent(dfsCapsEvent::GetCloudRootsREQ::construct);
	iUtils->registerEvent(dfsCapsEvent::GetCloudRootsRSP::construct);
	iUtils->registerEvent(dfsCapsEvent::GetReferrersREQ::construct);
	iUtils->registerEvent(dfsCapsEvent::GetReferrersRSP::construct);
	iUtils->registerEvent(dfsCapsEvent::RegisterCloudRoot::construct);
	iUtils->registerEvent(dfsCapsEvent::RegisterMyRefferrerNodeREQ::construct);
	iUtils->registerEvent(dfsReferrerEvent::ToplinkDeliverREQ::construct);
	iUtils->registerEvent(dfsReferrerEvent::ToplinkDeliverRSP::construct);
	iUtils->registerEvent(rpcEvent::IncomingOnAcceptor::construct);
	iUtils->registerEvent(rpcEvent::IncomingOnConnector::construct);
	iUtils->registerEvent(systemEvent::startService::construct);
	iUtils->registerEvent(timerEvent::TickAlarm::construct);
}
#endif
