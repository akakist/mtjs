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
// struct Hashable;
#include "hsh.h"

struct Cellable;



struct data_base : public Refcountable
{
    int type;
    data_base(int t): type(t) {}
    ~data_base()
    {
        // logErr2("~data_base() %d", type);
    }
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

extern std::vector< data_base* (*)()> db_constructors;

struct Cellable: public Refcountable
{
    Cellable()= delete;
    Cellable& operator=(const Cellable&) = delete;
    static Cellable* construct(Cellable *p, const std::string& id, const REF_getter<fee_calcer>& bc)
    {
        return new Cellable(p,id,bc);
    }
    ~Cellable()
    {
        // logErr2("~Cellable() %s",m_id.c_str());
    }

    const Cellable * parent=nullptr;
    const std::string m_id;
    std::set<REF_getter<fee_calcer>> calcers;

    // private:
    std::map<std::string,THASH_id > children_hashes;
    std::map<std::string, REF_getter<Cellable>> children_ptrs;
    std::string payload;
    int payload_ctor_idx=hsh::HSH_END;
    REF_getter<data_base> data=nullptr;
public:
    // std::set<Cellable*> accessed;
    bool accessed=false;

    Cellable(Cellable* _parent, const std::string & id, const REF_getter<fee_calcer>& bc): parent(_parent), m_id(id)
    {
        if(bc.valid())
        {
            calcers.insert(bc);
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
        o<<children_hashes;
        o<<payload;
        o<<payload_ctor_idx;
    }
    virtual void unpack(inBuffer& in)
    {
        int v=in.get_PN();
        // in>>m_id;
        in>>children_hashes;
        in>>payload;
        in>>payload_ctor_idx;
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


    REF_getter<Cellable> getLeafOrCreate(const std::string& id, IDatabase* db, const REF_getter<fee_calcer>& bc);
    REF_getter<Cellable> getLeafNoCreate(const std::string& id, IDatabase* db, const REF_getter<fee_calcer>& bc);

    void calc_tree_hash(_db_to_save &db_dump);

};
static const char* BASE62_TABLE[62] = {
    "0","1","2","3","4","5","6","7","8","9",
    "A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z",
    "a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z"
};
inline void appendRelativeInternalPath(std::vector<std::string>&vs, const std::string & k, int depth2)
{
    MUTEX_INSPECTOR;
    auto h=ghash(k.c_str());

    for(int i=0; i<depth2; i++)
    {
        auto k=h % 62;
        vs.push_back(BASE62_TABLE[k]);
        h/=62;
    }
    // return vs;
}


