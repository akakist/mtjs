#pragma once
#include "REF.h"
#include <set>
#include <string>
#include <rocksdb/db.h>
#include "blake2bHasher.h"
#include "commonError.h"
#include "IDatabase.h"
#include <vector>
#include "base62.h"
#include "db_to_save.h"
#include "fee_calcer.h"
#include "hsh.h"

struct Cellable;



struct data_base : public Refcountable
{
    int type;
    Cellable *parent;
    data_base(int t, Cellable* _parent): Refcountable("data_base"),
        type(t), parent(_parent) {}
    ~data_base()
    {
    }
    void setDirty(const REF_getter<fee_calcer>& bc);
    virtual void pack(outBuffer& o) const
    {
        o<<1;
        o<<type;
    }
    virtual void unpack(inBuffer& in)
    {
        int ver=in.get_PN();
        in>>type;
    }
    std::string getBuffer()
    {
        outBuffer o;
        pack(o);
        return o.asString()->container;
    }
    virtual std::string dump()=0;

};

extern std::vector< data_base* (*)(Cellable*)> db_constructors;

struct Cellable: public Refcountable
{

    Cellable()= delete;
    Cellable& operator=(const Cellable&) = delete;
    static Cellable* construct(Cellable *p, const std::string& id)
    {
        return new Cellable(p,id);
    }
    ~Cellable()
    {
    }

    Cellable * parent=nullptr;
    const std::string m_id;
    std::set<REF_getter<fee_calcer>> calcers_Z;

    std::map<std::string,THASH_id > children_hashes;
    std::map<std::string, REF_getter<Cellable>> children_ptrs;
    unsigned int payload_ctor_idx=hsh::HSH_END;
    REF_getter<data_base> data=nullptr;
    void print_calcers(std::vector<std::string> &v)
    {
        if(calcers_Z.size())
        {
            v.push_back(getDbId());
        }
        for(auto& z: children_ptrs)
        {
            z.second->print_calcers(v);
        }

    }
public:
    bool is_dirty=false;

    Cellable(Cellable* _parent, const std::string & id):Refcountable("cellable"),  parent(_parent), m_id(id)
    {
    }
    void setDirty(const REF_getter<fee_calcer>& bc)
    {
        is_dirty=true;
        if(bc.valid())
            calcers_Z.insert(bc);
        if(parent)
        {
            parent->setDirty(bc);
        }
    }
    std::string dump();

    std::string getDbId() const;

    void get_path(std::vector<const Cellable*> &s) const
    {
        s.push_back(this);
        if(parent)
        {
            parent->get_path(s);
        }
    }
    virtual void pack(outBuffer& o)
    {
        o<<1;
        // o<<m_id;
        o<<payload_ctor_idx;
        o<<children_hashes;
        bool valid=data.valid();
        o<<valid;
        if(valid)
            data->pack(o);
    }
    virtual void unpack(inBuffer& in)
    {
        int v=in.get_PN();
        // in>>m_id;
        in>>payload_ctor_idx;
        in>>children_hashes;
        bool valid;
        in>>valid;
        if(valid)
        {
            if(payload_ctor_idx<hsh::HSH_END)
            {
                data=db_constructors[payload_ctor_idx](this);
                data->unpack(in);
            }
            else
            {
                throw CommonError("!if(payload_ctor_idx<hsh::HSH_END)");;
            }

        }
    }
    std::string getBuffer()
    {
        outBuffer o;
        pack(o);
        return o.asString()->container;
    }
    const Cellable * get_root()
    {
        const Cellable *c=this;
        while(c->parent)
        {
            c=c->parent;
        }
        return c;
    }


    REF_getter<Cellable> getLeafOrCreate(const std::string& id, IDatabase* db);
    REF_getter<Cellable> getLeafNoCreate(const std::string& id, IDatabase* db);

    void calc_tree_hash(_db_to_save &db_dump);

};
// static const char* BASE62_TABLE[62] = {
//     "0","1","2","3","4","5","6","7","8","9",
//     "A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z",
//     "a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z"
// };
inline void appendRelativeInternalPath(std::vector<std::string>&vs, const std::string & k, int depth2)
{
    MUTEX_INSPECTOR;
    // auto h=ghash(k.c_str());
    if(k.size()<depth2)
    {
        throw CommonError("if(k.size()<depth2)");
    }
    // uint64_t h=0;
    for(int i=0; i<depth2; i++)
    {
        // auto k=h % 62;
        vs.push_back(k.substr(i, 1));
        // h/=62;
    }
}


