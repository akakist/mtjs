#include "scanService.h"
#include "Event/bcEvent.h"
#include "events_scanService.hpp"
#include "md/md_ScanREQ.h"
#include "md/md_ScanRSP.h"
#include "cellable.h"
#include "cells.h"
#include "hsh.h"
#include "CDatabaseRocksdb.h"
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <memory>
#include <time.h>
#include "tools_mt.h"

Scan::Service::Service(const SERVICE_id &id, const std::string &nm, IInstance *ins)
    : UnknownBase(nm), ListenerBuffered1Thread(nm, id), Broadcaster(ins) {}

Scan::Service::~Service() {}

bool Scan::Service::on_startService(const systemEvent::startService *) { return true; }

bool Scan::Service::ServiceInit(const bcEvent::ServiceInit *e)
{
    db_state = e->db;
    return true;
}

bool Scan::Service::ClientMsg(const bcEvent::ClientMsg *e)
{
    inBuffer in(e->msg);
    auto p = in.get_PN();
    REF_getter<MsgData::Base> b = msgFactory.create(p);
    b->unpack(in);
    auto hash = b->getHash();

    if (p == msgid::ScanREQ) {
        auto* pp = (MsgData::ScanREQ*)b.get();
        REF_getter<MsgData::ScanRSP> rsp = new MsgData::ScanRSP;
        // буфер 10 сек: scan_expired должен быть консервативным,
        // gc перепроверит на момент блока (t.block_timestamp)
        // `gc` перепроверит на t.block_timestamp — `-10` не нужен (clock skew нода/лидер)
        int64_t now = pp->now > 0 ? pp->now : (int64_t)time(NULL);
        int64_t max_sec = pp->max_sec > 0 ? pp->max_sec : 5;

        auto db = dynamic_cast<CDatabaseRocksdb*>(db_state.get());
        if (!db) {
            rsp->done = true;
            passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
            return true;
        }

        auto t0 = iUtils->getNow();
        std::unique_ptr<rocksdb::Iterator> it(db->db_ptr->NewIterator(rocksdb::ReadOptions()));
        if (pp->from.container.empty()) it->SeekToFirst();
        else it->Seek(pp->from.container);

        int64_t scanned = 0;
        while (it->Valid()) {
            if (iUtils->getNow() - t0 > max_sec * 1000000LL) break;
            auto key = it->key().ToString();
            if (key.size() == 32) {
                scanned++;
                auto val = it->value().ToString();
                Cellable c(nullptr, key);
                inBuffer vin(val);
                try { c.unpack_mx(vin); } catch (...) { it->Next(); continue; }
                if (c.data.valid()) {
                    bool expired = false;
                    switch (c.payload_ctor_idx) {
                    case hsh::bc_activity: {
                        auto* act = dynamic_cast<bc_activity*>(c.data.get());
                        if (act && act->ttl > 0 && act->_update_time + act->ttl < now)
                            expired = true;
                        break;
                    }
                    case hsh::bc_granule:
                    case hsh::bc_node:
                    case hsh::bc_address_state:
                    default:
                        // остальные типы удаляются ТОЛЬКО через bc_activity (gc_activity_apply)
                        break;
                    }
                    if (expired) {
                        THASH_id hh; hh.container = key;
                        rsp->dbids.push_back(hh);
                    }
                }
            }
            it->Next();
        }
        rsp->scanned = scanned;
        if (it->Valid()) rsp->next_from.container = it->key().ToString();
        else rsp->done = true;

        passEvent(new bcEvent::ClientMsgReply(hash, rsp->getBuffer(), poppedFrontRoute(e->route)));
        return true;
    }
    return true;
}

bool Scan::Service::handleEvent(const REF_getter<Event::Base> &e)
{
    switch (e->id) {
    case bcEventEnum::ServiceInit: return ServiceInit((const bcEvent::ServiceInit*)e.get());
    case bcEventEnum::ClientMsg:   return ClientMsg((const bcEvent::ClientMsg*)e.get());
    case systemEventEnum::startService: return on_startService((const systemEvent::startService*)e.get());
    case rpcEventEnum::IncomingOnAcceptor:
    case rpcEventEnum::IncomingOnConnector: {
        auto* ev = (const rpcEvent::IncomingOnAcceptor*)e.get();
        if (ev->e->id == bcEventEnum::ClientMsg) return ClientMsg((const bcEvent::ClientMsg*)ev->e.get());
        return false;
    }
    default: return false;
    }
}

void registerScanService(const char *pn)
{
    if (pn) {
        iUtils->registerPlugingInfo(pn, IUtils::PLUGIN_TYPE_SERVICE, ServiceEnum::Scan, "Scan", getEvents_scanService());
    } else {
        iUtils->registerService(ServiceEnum::Scan, Scan::Service::construct, "Scan");
        regEvents_scanService();
    }
}