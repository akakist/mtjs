#include "cellable.h"
#include "commonError.h"
#include "blake2bHasher.h"
#include <sstream>

std::string Cellable::getDbId() const
{
    if (parent)
        return parent->getDbId()  + m_id;

    return parent ? m_id : "";
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
            auto child_buf = c->getBuffer_mx();
            auto ch = blake2b_hash(child_buf);
            if (ch != children_hashes_mx[cid])
            {
                MUTEX_INSPECTOR;
                db_dump.add(c->getDbId(), child_buf);
                // c->last_size = child_buf.size();
                children_hashes_mx[cid] = ch;
            }
            lk.unlock();
        }
    }
    is_dirty = false;
}
void data_base::setDirty(Rollback* roll)
{
        MUTEX_INSPECTOR;

    // last_update_epoch=epoch;
    parent->setDirty__(roll);
}
