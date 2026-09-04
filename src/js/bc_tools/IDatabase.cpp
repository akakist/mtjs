#include "cellable.h"
#include "IDatabase.h"

REF_getter<Cellable> IDatabase::getLeafOrCreate(const REF_getter<Cellable>& _cur, const std::string &id, IDatabase *db, MutexLockerDeferred &l, Rollback* roll)
{
    MUTEX_INSPECTOR;
    auto cur=_cur;
    l.lock();
    auto it = cur->children_hashes_mx.find(id);
    if (it != cur->children_hashes_mx.end())
    {
        MUTEX_INSPECTOR;
        l.unlock();
        return getLeafNoCreate(cur, id, db,l);
    }
    // lk.unlock();
    if(roll)
    {
        if(!roll->data.count(cur.get()));
        {
            roll->data[cur.get()]=cur->getBuffer_mx();
        }
    }
    // l.lock();
    cur->children_hashes_mx[id].container = "";
    auto it2 = cur->children_ptrs_mx.find(id);
    if (it2 != cur->children_ptrs_mx.end())
        throw CommonError("if(it!=children_ptrs.end())");
    REF_getter<Cellable> c = new Cellable(cur.get(), id);
    cur->children_ptrs_mx.insert({id, c});
    l.unlock();
    c->setDirty__(roll);
    return c;
}

REF_getter<Cellable> IDatabase::getLeafNoCreate(const REF_getter<Cellable>&  _cur, const std::string &id, IDatabase *db, MutexLockerDeferred &l)
{
    MUTEX_INSPECTOR;
    auto cur=_cur;
    {
        MUTEX_INSPECTOR;
        l.lock();

    }
    auto ip = cur->children_ptrs_mx.find(id);
    if (ip != cur->children_ptrs_mx.end())
    {
        MUTEX_INSPECTOR;
        l.unlock();
        return ip->second;
    }
    auto it = cur->children_hashes_mx.find(id);
    if (it == cur->children_hashes_mx.end())
    {
        MUTEX_INSPECTOR;
        l.unlock();
        return NULL;
    }
    l.unlock();
    REF_getter<Cellable> cc = new Cellable(cur.get(), id);


    std::string result;
    auto dbId=cc->getDbId();
    int r = db->getGranule(dbId, &result);
    if (r)
        logErr2("db->getGranule err %s", cc->getDbId().c_str());

    if (result.size())
    {
        MUTEX_INSPECTOR;
        auto h = blake2b_hash(result);
        if (it->second == h)
        {
            inBuffer in(result);
            // l.lock();
            cc->unpack_mx(in);
            // /l.unlock();
        }
        else
        {
            db->clear_root = true;
            db->add_sync_out(dbId);
            // setDirty__(NULL);
            // return NULL;
            throw CommonError("getGranule: cell hash not matched %s %s granule size %ld body %s",
                 base16::encode(it->second.container).c_str(), 
                 base16::encode(h.container).c_str(), 
                 result.size(),
                base16::encode(result).c_str());
        }
    }
    {
        MUTEX_INSPECTOR;
        l.lock();
    }
    cur->children_ptrs_mx.insert({id, cc});
    {
        MUTEX_INSPECTOR;
        l.unlock();

    }
    return cc;
}
