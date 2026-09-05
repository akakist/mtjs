#include "cellable.h"
#include "IDatabase.h"

REF_getter<Cellable> IDatabase::getLeafOrCreate(const REF_getter<Cellable>& _cur, const std::string &id, MutexLockerDeferred &l, Rollback* roll)
{
    MUTEX_INSPECTOR;
    auto cur=_cur;
    l.lock();
    auto it = cur->children_hashes_mx.find(id);
    if (it != cur->children_hashes_mx.end())
    {
        MUTEX_INSPECTOR;
        l.unlock();
        return getLeafNoCreate(cur, id, l);
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

REF_getter<Cellable> IDatabase::getLeafNoCreate(const REF_getter<Cellable>&  _cur, const std::string &id, MutexLockerDeferred &l)
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
    int r = getGranule(dbId, &result);
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
            clear_root = true;
            add_sync_out(dbId);
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


REF_getter<Cellable> IDatabase::getByPathOrCreate(const REF_getter<Cellable>& _cur, const std::vector<std::string> &v, Rollback* roll)
{
    auto cur=_cur;
    for (auto &z : v)
    {
        MutexLockerDeferred l(cur->mx);
        cur = getLeafOrCreate(cur,z, l,roll);
    }
    return cur;
}
REF_getter<Cellable> IDatabase::getByPathOrCreate(const REF_getter<Cellable>& _cur, const std::deque<std::string> &v, Rollback* roll)
{
    auto cur=_cur;
    for (auto &z : v)
    {
        MutexLockerDeferred l(cur->mx);
        cur = getLeafOrCreate(cur,z, l,roll);
    }
    return cur;
}
REF_getter<Cellable> IDatabase::getByPathNoCreate(const REF_getter<Cellable>& _cur, const std::vector<std::string> &v)
{
    auto cur=_cur;
    // M_LOCK(cur->lock);
    for (auto &z : v)
    {
        MutexLockerDeferred l(cur->mx);
        cur = getLeafNoCreate(cur,z, l);
        if (!cur.valid())
            return NULL;
    }
    return cur;
}
std::vector<std::string> IDatabase::getPath(const std::string& name) 
{
    std::vector<std::string> out;
    auto h = blake2b_hash(name);
    out.reserve(5);
    out.emplace_back(h.container, 0, 1);
    out.emplace_back(h.container, 1, 1);
    out.emplace_back(h.container, 2, 1);
    out.emplace_back(h.container, 3, 1);
    out.emplace_back(h.container, 4, 28);
    return out;
}

std::vector<std::string> IDatabase::getContractPath(const CONTRACT_id &name)
{
    return getPath("CONTRACT_"+name.container);
}
std::vector<std::string> IDatabase::getContractDataPath(const CONTRACT_DATA_id &name)
{
    std::string key="CONTRACT_DATA_"+name.container;    
    return getPath(key);

}

std::vector<std::string> IDatabase::getNodePath(const NODE_id &name)
{
    return getPath("NODE_"+name.container);
}

REF_getter<bc_contract> IDatabase::getContract(const CONTRACT_id &name)
{
    MUTEX_INSPECTOR;
    auto cc = getByPathNoCreate(root.get(), getContractPath(name));
    if (!cc.valid())
        return NULL;
    if (!cc->data.valid())
        throw CommonError("if(!cc->data.valid())");
    return dynamic_cast<bc_contract *>(cc->data.get());
}
REF_getter<bc_contract_data> IDatabase::getContractData(const CONTRACT_DATA_id &name)
{
    MUTEX_INSPECTOR;
    auto cc = getByPathNoCreate(root.get(), getContractDataPath(name));
    if (!cc.valid())
        return NULL;
    if (!cc->data.valid())
        throw CommonError("if(!cc->data.valid())");
    return dynamic_cast<bc_contract_data *>(cc->data.get());
}
REF_getter<bc_nodelist> IDatabase::getNodeListOrCreate(Rollback* roll)
{
    MUTEX_INSPECTOR;
    auto cc = getByPathOrCreate(root.get(), getPath("NODE LIST"), roll);
    if (!cc.valid())
        return NULL;
    if (cc->data.valid())
    {
        return dynamic_cast<bc_nodelist *>(cc->data.get());
    }

    REF_getter<bc_nodelist> u = new bc_nodelist(cc.get());
    cc->data = u.get();
    cc->payload_ctor_idx = hsh::bc_nodelist;
    return u;
}
REF_getter<bc_nodelist> IDatabase::getNodeListNoCreate()
{
    MUTEX_INSPECTOR;
    auto cc = getByPathNoCreate(root.get(), getPath("NODE LIST"));
    if (!cc.valid())
        return NULL;
    if (cc->data.valid())
    {
        return dynamic_cast<bc_nodelist *>(cc->data.get());
    }

    return NULL;
}

REF_getter<bc_contract> IDatabase::addContract(const CONTRACT_id &name, Rollback* roll)
{
    MUTEX_INSPECTOR;
    auto cc = getByPathOrCreate(root.get(), getContractPath(name), roll);
    if (!cc.valid())
        throw CommonError("if(!cc.valid())");
    if (cc->data.valid())
        throw CommonError("if(cc->data.valid())");
    REF_getter<bc_contract> bc = new bc_contract(cc.get());
    cc->data = bc.get();
    cc->payload_ctor_idx = hsh::bc_contract;
    cc->data->setDirty(roll);
    return bc;
}
REF_getter<bc_contract_data> IDatabase::addContractData(const CONTRACT_DATA_id &name, Rollback* roll)
{
    MUTEX_INSPECTOR;
    auto cc = getByPathOrCreate(root.get(), getContractDataPath(name), roll);
    if (!cc.valid())
        throw CommonError("if(!cc.valid())");
    if (cc->data.valid())
        throw CommonError("if(cc->data.valid())");
    REF_getter<bc_contract_data> bc = new bc_contract_data(cc.get());
    cc->data = bc.get();
    cc->payload_ctor_idx = hsh::bc_contract_data;
    cc->data->setDirty(roll);
    return bc;
}

REF_getter<bc_values> IDatabase::getValuesOrCreate(Rollback* roll)
{
    MUTEX_INSPECTOR;
    auto r = this;
    MutexLockerDeferred lk(r->mx);
    auto l = getByPathOrCreate(root.get(), getPath("VALUES"),  roll);
    if (!l.valid())
        throw CommonError("if(!l.valid())");
    if (l->data.valid())
    {
        return dynamic_cast<bc_values *>(l->data.get());
    }

    REF_getter<bc_values> v = new bc_values(l.get());
    lk.lock();
    l->data = v.get();
    l->payload_ctor_idx = hsh::bc_values;
    lk.unlock();
    return v;
}
REF_getter<bc_values> IDatabase::getValuesNoCreate()
{
    MUTEX_INSPECTOR;
    auto r = this;
    MutexLockerDeferred lk(r->mx);
    auto l = getByPathNoCreate(root.get(), getPath("VALUES"));
    if(!l.valid())
        return NULL;
    if (l->data.valid())
    {
        return dynamic_cast<bc_values *>(l->data.get());
    }
    return NULL;

}

REF_getter<bc_values> IDatabase::checkValues()
{
    MUTEX_INSPECTOR;
    auto r = this;
    MutexLockerDeferred lk(r->mx);
    auto l = getByPathNoCreate(root.get(), getPath("VALUES"));
    if (!l.valid())
        return NULL;
    lk.lock();
    auto data=l->data;
    lk.unlock();
    if (data.valid())
    {
        return dynamic_cast<bc_values *>(data.get());
    }
    return NULL;
}

std::vector<std::string> IDatabase::getUserPath(const ADDRESS_id &addr)
{
    MUTEX_INSPECTORS("getUserPath");
    return getPath("USER_"+addr.addr);
}
std::vector<std::string> IDatabase::getAddressStatePath(const ADDRESS_id &addr)
{
    MUTEX_INSPECTOR;
    return getPath("USER_STATE_"+addr.addr);
}

REF_getter<bc_address_state> IDatabase::getAddressState(const ADDRESS_id &addr, Rollback* roll)
{
    MUTEX_INSPECTOR;
    auto cc = getByPathOrCreate(root.get(), getAddressStatePath(addr), roll);
    if (!cc.valid())
        return NULL;
    if (cc->data.valid())
    {
        return dynamic_cast<bc_address_state *>(cc->data.get());
    }

    REF_getter<bc_address_state> u = new bc_address_state(cc.get());
    cc->data = u.get();
    cc->payload_ctor_idx = hsh::bc_address_state;
    return u;
}
REF_getter<bc_address_state> IDatabase::checkUserState(const ADDRESS_id &addr)
{
    MUTEX_INSPECTOR;
    auto cc = getByPathNoCreate(root.get(), getAddressStatePath(addr));
    if (!cc.valid())
        return NULL;

    return dynamic_cast<bc_address_state *>(cc->data.get());
}

std::vector<REF_getter<bc_node>> IDatabase::getAllNodes()
{
    MUTEX_INSPECTOR;
    auto nl=getNodeListNoCreate();
    if(!nl.valid())
        throw CommonError("if(!nl.valid())");

    std::vector<REF_getter<bc_node>> vv;
    auto ll=nl->getList();
    for(auto& z: ll)
    {
        vv.push_back(getNode(z));
    }
    return vv;

}
REF_getter<bc_node> IDatabase::addNode(const NODE_id &name, Rollback* roll)
{
    MUTEX_INSPECTOR;

    auto nl=getNodeListOrCreate(roll);
    // if(!nl.valid())
    //     throw CommonError("if(!nl.valid())");
    if(nl->count(name))
        throw CommonError("if(!nl->list.count(name))");
    nl->insert(name);
    nl->setDirty(roll);

    auto cc = getByPathOrCreate(root.get(), getNodePath(name), roll);

    if (cc->data.valid())
        throw CommonError("if(cc->data.valid())");

        REF_getter<bc_node> n = new bc_node(cc.get());
    cc->data = n.get();
    cc->payload_ctor_idx = hsh::bc_node;
    return n;
}

REF_getter<bc_node> IDatabase::getNode(const NODE_id &name)
{
    MUTEX_INSPECTOR;

    auto cc = getByPathNoCreate(root.get(), getNodePath(name));
    if (!cc.valid())
        return NULL;
    if (cc->data.valid())
        return dynamic_cast<bc_node *>(cc->data.get());
    else
        throw CommonError("if(cc->data.valid())");
}

REF_getter<MsgData::BlockAcceptedREQ> load_last_block(IDatabase *db)
{
    std::string buf;
    auto r=db->getGranule(".last_block",&buf);
    REF_getter<MsgData::BlockAcceptedREQ> last_block;
    if(buf.size())
    {
        last_block=new MsgData::BlockAcceptedREQ;
        inBuffer in(buf);
        // M_LOCK(r->mx);
        last_block->unpack2(in);

    }
    return last_block;
    // std::string pn=db->getDbName()+".last_block";
    // auto buf=iUtils->load_file_no_throw(pn);
    // if(buf.size())
    // {
    //     logErr2("buf.size() %d",buf.size());
    //     last_block=new MsgData::BlockAcceptedREQ;
    //     inBuffer in(buf);
    //     // M_LOCK(r->mx);
    //     last_block->unpack2(in);
    // }
    // return last_block;
}
REF_getter<Cellable>  getRoot(IDatabase *db, const REF_getter<MsgData::BlockAcceptedREQ>& pb)
{
    MUTEX_INSPECTOR;

    REF_getter<Cellable> r = new Cellable(NULL,"");
    REF_getter<MsgData::BlockAcceptedREQ> last_block;
    std::string root_cell;
    {
        MUTEX_INSPECTOR;
        std::string lb;
        int err = db->getGranule("", &lb);
        
        if(!err && lb.size() && pb.valid())
        {
            MUTEX_INSPECTOR;
            auto h=blake2b_hash(lb);
            if(pb->blockInfo->new_root_hash1!=h)
            {
                inBuffer in(lb);
                r->unpack_mx(in);
            }
            else{
                 logErr2("block hash not matched");
                 return new Cellable(NULL,"");
            }
            
        }


    }

    return r;
}
