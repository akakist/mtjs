#include "cellable.h"


std::string Cellable::getDbId() const
{
    if(parent)
        return parent->getDbId()+"."+m_id;

    return parent?m_id:"";
}

std::string Cellable::dump()
{
    std::ostringstream str;
    str<<"Cellable "<< std::endl
       // << "payload_ctor_idx="<<payload_ctor_idx << std::endl
       <<"children:" << std::endl;
    for(auto &z: children_hashes)
    {
        str<<"<a href='"<<z.first<< "/'>" << z.first << "</a>" << " -> "<< base62::encode(z.second.container) << std::endl;
    }
    if(data.valid())
        str << data->dump();
    return str.str();

}

REF_getter<Cellable> Cellable::getLeafOrCreate(const std::string& id, IDatabase* db, const REF_getter<fee_calcer>& bc)
{
    MUTEX_INSPECTOR;
    if(bc.valid())
    {
        calcers.insert(bc);
    }
    accessed=true;
    auto it=children_hashes.find(id);
    if(it!=children_hashes.end())
        return getLeafNoCreate(id,db,bc);


    children_hashes[id].container="";
    auto it2=children_ptrs.find(id);
    if(it2!=children_ptrs.end())
        throw CommonError("if(it!=children_ptrs.end())");
    REF_getter<Cellable> c=new Cellable(this,id,bc);
    children_ptrs.insert({id,c});
    c->accessed=true;
    return c;
}

REF_getter<Cellable> Cellable::getLeafNoCreate(const std::string& id, IDatabase* db, const REF_getter<fee_calcer>& bc)
{
    MUTEX_INSPECTOR;
    if(bc.valid())
    {
        calcers.insert(bc);
    }
    accessed=true;
    auto it=children_hashes.find(id);
    if(it==children_hashes.end())
        return NULL;


    auto ip=children_ptrs.find(id);
    if(ip!=children_ptrs.end())
    {
        ip->second->accessed=true;
        if(bc.valid())
        {
            ip->second->calcers.insert(bc);
        }

        return ip->second;

    }
    // logErr2("getLeafNoCreate %s create new Cellable", id.c_str());
    REF_getter<Cellable> cc=new Cellable(this,id,bc);
    cc->accessed=true;

    children_ptrs.insert({id,cc});
    cc->accessed=true;

    std::string result;

    int r=db->get_cell(cc->getDbId(),&result);
    if(r)
        logErr2("db->get_cell err %s",cc->getDbId().c_str());

    // logErr2("getLeafNoCreate %s loaded result %d", id.c_str(), result.size());

    if(result.size())
    {
        auto h=blake2b_hash(result);
        if(it->second==h)
        {
            // logErr2("getLeafNoCreate %s hash ok", id.c_str());

            inBuffer in(result);
            cc->unpack(in);
            if(cc->payload.size() && cc->payload_ctor_idx<hsh::HSH_END)
            {
                // logErr2("getLeafNoCreate %s create data cc->payload_ctor_idx %d", id.c_str(),cc->payload_ctor_idx);

                cc->data=db_constructors[cc->payload_ctor_idx]();
                inBuffer iz(cc->payload);
                cc->data->unpack(iz);
            }
        }
        else
        {
            throw CommonError("get_cell: cell hash not matched %s %s", base62::encode(it->second.container).c_str(),base62::encode(h.container).c_str());

        }
    }
    return cc;
}
void Cellable::calc_tree_hash(_db_to_save &db_dump)
{
    // logErr2("calc_tree_hash %s accessed %d",getDbId().c_str(), accessed);
    if(!accessed)
        return;
    if(data.valid())
    {
        payload=data->getBuffer();
        // data=NULL;
    }
    for(auto &zz:children_ptrs)
    {
        auto cid=zz.first;
        auto c=zz.second;
        if(c->accessed)
        {

            c->calc_tree_hash(db_dump);
            for(auto& bc: c->calcers)
            {
                if(bc.valid())
                    calcers.insert(bc);
            }
            auto child_buf=c->getBuffer();
            auto ch=blake2b_hash(child_buf);
            if(ch!=children_hashes[cid])
            {
                // logErr2("if(ch!=children_hashes[cid]) %s %s %s",c->getDbId().c_str(), base62::encode(ch).c_str(),base62::encode(children_hashes[cid]).c_str());
                db_dump.add(c->getDbId(),child_buf);
                if(c->calcers.size()>0)
                {
                    auto portion=child_buf.size()/c->calcers.size();
                    for(auto& bc: c->calcers)
                    {
                        if(bc.valid())
                        {
                            bc->add(portion);
                            calcers.insert(bc);
                        }
                    }
                    c->calcers.clear();
                }
                children_hashes[cid]=ch;
            }
            // c->accessed=false;
        }

        // children_ptrs.erase(cid);
    }
    accessed=false;
}
