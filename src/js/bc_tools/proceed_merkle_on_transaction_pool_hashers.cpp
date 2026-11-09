#include "THASH_id.h"
#include "cellable.h"
#include "db_to_save.h"
#include "blake2bHasher.h"

THASH_id proceed_merkle_on_transaction_pool_hashers(const REF_getter<Cellable> &r, _db_to_save &db_to_save_Z)
{
    MUTEX_INSPECTOR;
    r->calc_tree_hash(db_to_save_Z);

    std::string root_buf;
    {
        
        root_buf = r->getBuffer_mx();
    }
    auto root_hash = blake2b_hash(root_buf);
    db_to_save_Z.add("", root_buf);
    THASH_id ret;
    ret.container = root_hash.container;
    return ret;
}
