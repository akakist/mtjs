#include "Events/System/Net/rpcEvent.h"
#include "Events/System/Run/startServiceEvent.h"
#include "IUtils.h"
#include "Events/System/timerEvent.h"
#include "REF.h"
#include "IUtils.h"
#include "commonError.h"
#include "SERVICE_id.h"
#include "IInstance.h"
#include "broadcaster.h"
#include "corelib/mutexInspector.h"
#include "Event/bcEvent.h"
#include <exception>
#include <string>
#include <time.h>
#include "grainReaderService.h"
#include "event_mt.h"
#include "listenerBuffered1Thread.h"
#include "ioBuffer.h"
#include "msg.h"
#include "tools_mt.h"
#include "events_grainReaderService.hpp"
#include "unknown.h"
#include "msgFactory.h"
#include "md/md_GetUserNonceREQ.h"
#include "md/md_GetUserNonceRSP.h"
#include "init_root.h"
#include "CDatabaseRocksdb.h"
#include "md/md_GetNodeInfoREQ.h"
#include "md/md_GetNodeInfoRSP.h"
#include "md/md_GetContractDataREQ.h"
#include "md/md_GetContractDataRSP.h"
#include "md/md_GetValuesREQ.h"
#include "md/md_GetValuesRSP.h"
#include "md/md_GetContractREQ.h"
#include "md/md_GetContractRSP.h"
#include "md/md_GetStateREQ.h"
#include "md/md_GetStateRSP.h"
#include "md/md_GetAddressInfoREQ.h"
#include "md/md_GetAddressInfoRSP.h"

bool GrainReader::Service::on_startService(const systemEvent::startService *)
{
    MUTEX_INSPECTOR;

    return true;
}

bool GrainReader::Service::on_timer(const timerEvent::TickTimer *e)
{
    MUTEX_INSPECTOR;
    return true;
}
bool GrainReader::Service::on_alarm(const timerEvent::TickAlarm *e)
{
    MUTEX_INSPECTOR;
    return false;
}

bool GrainReader::Service::handleEvent(const REF_getter<Event::Base> &e)
{
    MUTEX_INSPECTOR;
    XTRY;
    try
    {
        MUTEX_INSPECTOR;
        auto &ID = e->id;
        switch (ID)
        {
        case bcEventEnum::ClientMsg:
            return ClientMsg((const bcEvent::ClientMsg *)e.get());
        case bcEventEnum::ServiceInit:
            return ServiceInit((const bcEvent::ServiceInit *)e.get());
        case timerEventEnum::TickTimer:
            return on_timer((const timerEvent::TickTimer *)e.get());
        case timerEventEnum::TickAlarm:
            return on_alarm((const timerEvent::TickAlarm *)e.get());
        case systemEventEnum::startService:
            return on_startService((const systemEvent::startService *)e.get());
        case rpcEventEnum::IncomingOnAcceptor:
        {
            const rpcEvent::IncomingOnAcceptor *ev = static_cast<const rpcEvent::IncomingOnAcceptor *>(e.get());
            auto &IDA = ev->e->id;

            switch (IDA)
            {
            case bcEventEnum::ClientMsg:
                return ClientMsg((const bcEvent::ClientMsg *)ev->e.get());
            default:
                throw CommonError("unhabdled ev %d %s", IDA, iUtils->genum_name(IDA));
            }
        }
        break;
        case rpcEventEnum::IncomingOnConnector:
        {
            const rpcEvent::IncomingOnConnector *ev = static_cast<const rpcEvent::IncomingOnConnector *>(e.get());
            auto &IDC = ev->e->id;
            switch (IDC)
            {
            case bcEventEnum::ClientMsg:
                return ClientMsg((const bcEvent::ClientMsg *)ev->e.get());

            default:
                throw CommonError("unhabdled ev %d %s", IDC, iUtils->genum_name(IDC));
            }

        }
        break;
        default:
            throw CommonError("unhabdled ev %d %s", ID, iUtils->genum_name(ID));
        }
    }
    catch (std::exception &e)
    {
        logErr2("GrainReader std::exception  %s", e.what());
    }
    XPASS;
    return false;
}


GrainReader::Service::~Service()
{
}

GrainReader::Service::Service(const SERVICE_id &id, const std::string &nm, IInstance *ins)
    : UnknownBase(nm),
      ListenerBuffered1Thread(nm, id),
      Broadcaster(ins)
{
}
bool GrainReader::Service::ServiceInit(const bcEvent::ServiceInit *e)
{
    conf = e;
    db_state_3=e->db;
    return true;
}

bool GrainReader::Service::ClientMsg(const bcEvent::ClientMsg *e)
{

    MUTEX_INSPECTOR;
    inBuffer in(e->msg);

    auto p = in.get_PN();
    REF_getter<MsgData::Base> b=msgFactory.create(p);
    b->unpack(in);
    auto hash=b->getHash();

    switch(p)
    {
    case msgid::GetAddressInfoREQ: {
        auto pp = (MsgData::GetAddressInfoREQ*)b.get();
        REF_getter<MsgData::GetAddressInfoRSP> rsp = new MsgData::GetAddressInfoRSP;
        // rsp->addr = pp->addr;
        auto u = db_state_3->getAddressStateNoCreate(pp->addr);   // normalize — в getPath
        if (u.valid()) {
            rsp->found = true;
            rsp->balance = u->balance;
            rsp->nonce = u->nonce;
            // rsp->name = pp->addr.addr;
            for (auto& [id, k] : u->keys) {
                MsgData::KeyInfo ki;
                ki.id = id;
                ki.type = k.type;
                ki.keybuffer = base16::encode(k.keybuffer);
                rsp->keys.push_back(ki);
            }
        }
        passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
        break;
    }
    case msgid::GetUserNonceREQ:
    {
        auto pp=(MsgData::GetUserNonceREQ*) b.get();
        // ▼▼▼ NoCreate — не создаём адрес (спам RPC)
        auto u = db_state_3->getAddressStateNoCreate(pp->user_address);
        logErr2("case msgid::GetUserNonceREQ: pp->user_address %s",pp->user_address.str().c_str());
        REF_getter<MsgData::GetUserNonceRSP> rsp = new MsgData::GetUserNonceRSP;
        if (u.valid()) {
        logErr2("case msgid::GetUserNonceREQ: pp->user_address %s found",pp->user_address.str().c_str());
            M_LOCK(u->parent->mx);
            rsp->nonce = u->nonce;
            rsp->balance = u->balance;
            rsp->found = true;
        } else {
        logErr2("case msgid::GetUserNonceREQ: pp->user_address %s !found",pp->user_address.str().c_str());
            rsp->nonce = 0;
            rsp->balance = 0;
            rsp->found = false;
        }
        passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
        // ▲▲▲
    }
    break;
    case msgid::GetNodeInfoREQ:
    {
        auto pp=(MsgData::GetNodeInfoREQ*) b.get();
        REF_getter<MsgData::GetNodeInfoRSP> rsp = new MsgData::GetNodeInfoRSP;
        auto n = db_state_3->getNodeNoCreate(pp->node_name);
        if (n.valid()) {
            rsp->found = true;
            rsp->name = n->getName();
            rsp->owner = n->get_owner();
            rsp->ed_pk = base16::encode(n->get_ed_pk());
            rsp->bls_pk = base16::encode(n->get_bls_pk().serialize());
            rsp->ip = n->get_ip();
            rsp->enabled = n->isEnabled();
            rsp->missed_rounds = n->missed_rounds;
            rsp->full_stake = n->get_full_stake();
            rsp->stakes = n->getStakes();
        }
        passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
        break;
    }
    case msgid::GetValuesREQ:
    {
        REF_getter<MsgData::GetValuesRSP> rsp = new MsgData::GetValuesRSP;
        auto v = db_state_3->getValuesNoCreate();
        if (v.valid()) {
            rsp->found = true;
            rsp->fees = v->values;
        }
        passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
        break;
    }
    case msgid::GetContractDataREQ:
    {
        auto pp=(MsgData::GetContractDataREQ*) b.get();
        REF_getter<MsgData::GetContractDataRSP> rsp = new MsgData::GetContractDataRSP;
        rsp->contract = pp->contract.container;
        rsp->key = pp->key.container;
        auto d = db_state_3->getContractDataNoCreate(pp->contract, pp->key);
        if (d.valid()) {
            rsp->found = true;
            rsp->value = d->container;
        }
        passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
        break;
    }
    case msgid::GetContractREQ:
    {
        auto pp=(MsgData::GetContractREQ*) b.get();
        REF_getter<MsgData::GetContractRSP> rsp = new MsgData::GetContractRSP;
        rsp->contract = pp->contract;
        auto c = db_state_3->getContractNoCreateConst(pp->contract);
        if (c.valid()) {
            rsp->found = true;
            rsp->owners = c->owners;
            rsp->src = c->src;
        }
        passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
        break;
    }

// в ClientMsg switch(p):
    case msgid::GetStateREQ:
    {
        REF_getter<MsgData::GetStateRSP> rsp = new MsgData::GetStateRSP;
        auto st = db_state_3->getStateNoCreateConst();
        if (st.valid()) {
            rsp->found = true;
            rsp->total_supply = st->total_supply.str();
            rsp->total_stake  = st->total_stake.str();
            rsp->account_count = st->account_count;
            rsp->node_count = st->node_count;
            rsp->contract_count = st->contract_count;
            rsp->tx_count = st->tx_count;
            rsp->block_height = st->block_height;
            rsp->genesis_time = st->genesis_time;
            rsp->chain_id = st->chain_id;
        }
        passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
        break;
    }
    default:
        logErr2("unhanddledf msg %s",msgName(p));
    }

    return true;
}

void registerGrainReaderService(const char *pn)
{
    MUTEX_INSPECTOR;
    /// регистрация в фабрике сервиса и событий

    XTRY;
    if (pn)
    {
        iUtils->registerPlugingInfo(pn, IUtils::PLUGIN_TYPE_SERVICE, ServiceEnum::GrainReader, "GrainReader", getEvents_grainReaderService());
    }
    else
    {
        iUtils->registerService(ServiceEnum::GrainReader, GrainReader::Service::construct, "GrainReader");
        regEvents_grainReaderService();
    }

    XPASS;
}
