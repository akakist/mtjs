#include "cellable.h"
#include "commonError.h"
#include "blake2bHasher.h"
#include <sstream>

std::string Cellable::getDbId() const
{
    if (parent)
        return parent->getDbId() + "." + m_id;

    return parent ? m_id : "";
}

std::string Cellable::dump()
{
    std::ostringstream str;
    str << "Cellable " << std::endl
        << "children:" << std::endl;
    {
        MutexLocker lk(mx);
        for (auto &z : children_hashes_mx)
        {
            str << "<a href='" << z.first << "/'>" << z.first << "</a>" << " -> " << base16::encode(z.second.container) << std::endl;
        }
    }
    if (data.valid())
        str << data->dump();
    return str.str();
}

REF_getter<Cellable> Cellable::getLeafOrCreate(const std::string &id, IDatabase *db, MutexLockerDeferred &l)
{
    MUTEX_INSPECTOR;
    {
        MUTEX_INSPECTOR;
        l.lock();

    }
    auto it = children_hashes_mx.find(id);
    if (it != children_hashes_mx.end())
    {
        MUTEX_INSPECTOR;
        l.unlock();
        return getLeafNoCreate(id, db,l);
    }
    // lk.unlock();

    // l.lock();
    children_hashes_mx[id].container = "";
    auto it2 = children_ptrs_mx.find(id);
    if (it2 != children_ptrs_mx.end())
        throw CommonError("if(it!=children_ptrs.end())");
    REF_getter<Cellable> c = new Cellable(this, id);
    children_ptrs_mx.insert({id, c});
    {
        MUTEX_INSPECTOR;
        l.unlock();

    }
    return c;
}

REF_getter<Cellable> Cellable::getLeafNoCreate(const std::string &id, IDatabase *db, MutexLockerDeferred &l)
{
    MUTEX_INSPECTOR;
    {
        MUTEX_INSPECTOR;
        l.lock();

    }
    auto ip = children_ptrs_mx.find(id);
    if (ip != children_ptrs_mx.end())
    {
        MUTEX_INSPECTOR;
        l.unlock();
        return ip->second;
    }
    auto it = children_hashes_mx.find(id);
    if (it == children_hashes_mx.end())
    {
        MUTEX_INSPECTOR;
        l.unlock();
        return NULL;
    }
    {
        MUTEX_INSPECTOR;
        l.unlock();

    }
    REF_getter<Cellable> cc = new Cellable(this, id);


    std::string result;

    int r = db->get_cell(cc->getDbId(), &result);
    if (r)
        logErr2("db->get_cell err %s", cc->getDbId().c_str());

    if (result.size())
    {
        MUTEX_INSPECTOR;
        auto h = blake2b_hash(result);
        if (it->second == h)
        {
            inBuffer in(result);
            cc->unpack(in);
        }
        else
        {
            throw CommonError("get_cell: cell hash not matched %s %s", base16::encode(it->second.container).c_str(), base16::encode(h.container).c_str());
        }
    }
    {
        MUTEX_INSPECTOR;
        l.lock();
    }
    children_ptrs_mx.insert({id, cc});
    {
        MUTEX_INSPECTOR;
        l.unlock();

    }
    return cc;
}
void Cellable::calc_tree_hash(_db_to_save &db_dump)
{
    MUTEX_INSPECTOR;
    if (!is_dirty)
        return;
    MutexLockerDeferred lk(mx);
    lk.lock();
    auto cptr_copy = children_ptrs_mx;
    lk.unlock();

    for (auto &zz : cptr_copy)
    {
        MUTEX_INSPECTOR;
        auto cid = zz.first;
        auto c = zz.second;
        if (c->is_dirty)
        {
            MUTEX_INSPECTOR;
            // lk.unlock();
            c->calc_tree_hash(db_dump);
            lk.lock();
            auto child_buf = c->getBuffer();
            auto ch = blake2b_hash(child_buf);
            if (ch != children_hashes_mx[cid])
            {
                MUTEX_INSPECTOR;
                db_dump.add(c->getDbId(), child_buf);
                c->last_size = child_buf.size();
                children_hashes_mx[cid] = ch;
            }
            lk.unlock();
        }
    }
    is_dirty = false;
}
void data_base::setDirty()
{
    parent->setDirty();
}
