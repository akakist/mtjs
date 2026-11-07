#include "Events/System/Net/rpcEvent.h"
#include "Events/System/timerEvent.h"
#include "THASH_id.h"
#include "corelib/mutexInspector.h"
#include "txValidatorService.h"
#include "ioBuffer.h"
#include "tools_mt.h"
#include "events_txValidatorService.hpp"
#include "cells.h"

bool TxValidator::Service::on_startService(const systemEvent::startService *)
{
    MUTEX_INSPECTOR;

    return true;
}

bool TxValidator::Service::on_timer(const timerEvent::TickTimer *e)
{
    MUTEX_INSPECTOR;
    return true;
}
bool TxValidator::Service::on_alarm(const timerEvent::TickAlarm *e)
{
    MUTEX_INSPECTOR;
    return false;
}

bool TxValidator::Service::handleEvent(const REF_getter<Event::Base> &e)
{
    MUTEX_INSPECTOR;
    XTRY;
    try
    {
        MUTEX_INSPECTOR;
        auto &ID = e->id;
        switch (ID)
        {
        case bcEventEnum::AddTxREQ:
            return AddTxREQ((const bcEvent::AddTxREQ *)e.get());
        case bcEventEnum::ClientMsg:
            return ClientMsg((const bcEvent::ClientMsg *)e.get());
        case bcEventEnum::ServiceInit:
            return ServiceInit((const bcEvent::ServiceInit *)e.get());
        case bcEventEnum::InvalidateRoot:
            return InvalidateRoot((const bcEvent::InvalidateRoot *)e.get());
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
            case bcEventEnum::AddTxREQ:
                return AddTxREQ((const bcEvent::AddTxREQ *)ev->e.get());
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
            case bcEventEnum::AddTxREQ:
                return AddTxREQ((const bcEvent::AddTxREQ *)ev->e.get());
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
        logErr2("TxValidator std::exception  %s", e.what());
    }
    XPASS;
    return false;
}

TxValidator::Service::~Service()
{
}

TxValidator::Service::Service(const SERVICE_id &id, const std::string &nm, IInstance *ins)
    : UnknownBase(nm),
      ListenerBuffered1Thread(nm, id),
      Broadcaster(ins)
{
}

#include "s_key.h"
#include "bc_address_state.h"
#include <yyjson.h>

bool TxValidator::Service::ServiceInit(const bcEvent::ServiceInit *e)
{
    logErr2("TxValidator::Service::ServiceInit");
    MUTEX_INSPECTOR;
    conf = e;
    db = e->db;
    auto prev=load_last_block(db.get());
    xroot=getRoot(db.get(),prev);
    return true;
}
bool TxValidator::Service::InvalidateRoot(const bcEvent::InvalidateRoot *e)
{
    // logErr2("TxValidator::Service::InvalidateRoot");
    MUTEX_INSPECTOR;
    auto prev=load_last_block(db.get());
    xroot=getRoot(db.get(),prev);
    return true;
}

bool TxValidator::Service::AddTxREQ(const bcEvent::AddTxREQ *e)
{
    MUTEX_INSPECTOR;
    std::optional<std::string> err;
    auto hash = e->tx->getHash();

    yyjson_doc* doc = yyjson_read(e->tx->tx_body.data(), e->tx->tx_body.size(), 0);
    if (!doc) { err = "tx_body: invalid json"; }
    else {
        yyjson_val *root = yyjson_doc_get_root(doc);
        yyjson_val *sv = root ? yyjson_obj_get(root, "sender") : nullptr;
        yyjson_val *ki = root ? yyjson_obj_get(root, "key_id") : nullptr;
        std::string sender_hex;
        if (sv && yyjson_is_str(sv)) sender_hex = yyjson_get_str(sv);
        int64_t key_id = -1;
        if (ki && yyjson_is_num(ki)) key_id = yyjson_get_sint(ki);

        if (!sender_hex.empty()) {
            ADDRESS_id senderAddress;
            senderAddress.addr = base16::decode(sender_hex);
            auto u = cells::getAddressStateNoCreate(xroot.get(),db.get(), senderAddress);
            if (!u.valid()) { err = "sender address not found"; }
            else if (key_id < 0) { err = "key_id required"; }
            else {
                auto k = u->getKey((uint64_t)key_id);
                if (!k) { err = "key_id not found"; }
                else if (!verify(*k, e->tx->sig_ed_bin, hash.container)) { err = "verify failed"; }
                else {
                    e->tx->sender_addr = senderAddress;
                    e->tx->sender_key_id = (uint64_t)key_id;
                    e->tx->sender_resolved = true;
                }
            }
        } else {
            if (!e->tx->verify()) err = "verify failed (legacy ed25519)";
        }
        yyjson_doc_free(doc);
    }

    if (!err) sendEvent(ServiceEnum::Node, new bcEvent::PutTransactionREQ(e->tx, this));
    passEvent(new bcEvent::AddTxRSP(hash, err.has_value(), err ? *err : "transaction added to pool",
                                    poppedFrontRoute(e->route)));
    return true;
}

bool TxValidator::Service::ClientMsg(const bcEvent::ClientMsg *e)
{
    logErr2("TxValidator ClientMsg");
    MUTEX_INSPECTOR;
    inBuffer in(e->msg);

    auto p = in.get_PN();
    THASH_id hash;
    hash = blake2b_hash(e->msg);
    switch (p)
    {

    default:
        throw CommonError("unhandled msgid e. ff %d", p);
    }

    return true;
}

void registerTxValidatorService(const char *pn)
{
    MUTEX_INSPECTOR;
    /// регистрация в фабрике сервиса и событий

    XTRY;
    if (pn)
    {
        iUtils->registerPlugingInfo(pn, IUtils::PLUGIN_TYPE_SERVICE, ServiceEnum::TxValidator, "TxValidator", getEvents_txValidatorService());
    }
    else
    {
        iUtils->registerService(ServiceEnum::TxValidator, TxValidator::Service::construct, "TxValidator");
        regEvents_txValidatorService();
    }
    XPASS;
}
