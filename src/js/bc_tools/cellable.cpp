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
       <<"children:" << std::endl;
    for(auto &z: children_hashes)
    {
        str<<"<a href='"<<z.first<< "/'>" << z.first << "</a>" << " -> "<< base16::encode(z.second.container) << std::endl;
    }
    if(data.valid())
        str << data->dump();
    return str.str();

}

REF_getter<Cellable> Cellable::getLeafOrCreate(const std::string& id, IDatabase* db)
{
    MUTEX_INSPECTOR;
    auto it=children_hashes.find(id);
    if(it!=children_hashes.end())
        return getLeafNoCreate(id,db);


    children_hashes[id].container="";
    auto it2=children_ptrs.find(id);
    if(it2!=children_ptrs.end())
        throw CommonError("if(it!=children_ptrs.end())");
    REF_getter<Cellable> c=new Cellable(this,id);
    children_ptrs.insert({id,c});
    return c;
}

REF_getter<Cellable> Cellable::getLeafNoCreate(const std::string& id, IDatabase* db)
{
    MUTEX_INSPECTOR;
    auto it=children_hashes.find(id);
    if(it==children_hashes.end())
        return NULL;


    auto ip=children_ptrs.find(id);
    if(ip!=children_ptrs.end())
    {

        return ip->second;

    }
    REF_getter<Cellable> cc=new Cellable(this,id);

    children_ptrs.insert({id,cc});

    std::string result;

    int r=db->get_cell(cc->getDbId(),&result);
    if(r)
        logErr2("db->get_cell err %s",cc->getDbId().c_str());


    if(result.size())
    {
        auto h=blake2b_hash(result);
        if(it->second==h)
        {
            inBuffer in(result);
            cc->unpack(in);
        }
        else
        {
            throw CommonError("get_cell: cell hash not matched %s %s", base16::encode(it->second.container).c_str(),base16::encode(h.container).c_str());

        }
    }
    return cc;
}
void Cellable::calc_tree_hash(_db_to_save &db_dump)
{
    if(!is_dirty)
        return;
    for(auto &zz:children_ptrs)
    {
        auto cid=zz.first;
        auto c=zz.second;
        if(c->is_dirty)
        {

            c->calc_tree_hash(db_dump);
            for(auto& bc: c->calcers_Z)
            {
                if(bc.valid())
                    calcers_Z.insert(bc);
            }
            auto child_buf=c->getBuffer();
            auto ch=blake2b_hash(child_buf);
            if(ch!=children_hashes[cid])
            {
                db_dump.add(c->getDbId(),child_buf);
                c->last_size=child_buf.size();
                if(c->calcers_Z.size()>0)
                {
                    auto portion=child_buf.size()/c->calcers_Z.size();
                    for(auto& bc: c->calcers_Z)
                    {
                        if(bc.valid())
                        {
                            bc->add(portion);
                            calcers_Z.insert(bc);
                        }
                    }
                    c->calcers_Z.clear();
                }
                children_hashes[cid]=ch;
            }
        }

    }
    is_dirty=false;
}
void data_base::setDirty(const REF_getter<fee_calcer>& bc)
{
    parent->setDirty(bc);
}
