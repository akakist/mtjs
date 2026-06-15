#pragma once
#include "REF.h"
#include <string>
#include <rocksdb/db.h>
#include "commonError.h"
#include "IDatabase.h"
#include <vector>
#include "db_to_save.h"
#include "hsh.h"
#include "THASH_id.h"

struct Cellable;



struct data_base : public Refcountable
{
    int type;
    Cellable *parent;

    time_t create_time=0;
    int ttl=-1;

    data_base(int t, Cellable* _parent, time_t _create_time, int _ttl ): Refcountable("data_base"),
        type(t), parent(_parent), create_time(_create_time),ttl(_ttl) {}
    ~data_base()
    {
    }
    void setDirty();
    virtual void pack(outBuffer& o) const
    {
        o<<1;
        o<<type;
        o<<create_time<<ttl;
    }
    virtual void unpack(inBuffer& in)
    {
        int ver=in.get_PN();
        in>>type;
        in>>create_time>>ttl;
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
    Mutex mx;
    /// @brief  не изменяется, выставляется только в конструкторе
    Cellable * parent=nullptr;

    /// @brief  не изменяется, выставляется только в конструкторе
    const std::string m_id;

    /// @brief  выставляется только в потоке ноды, мутекс не нужен
    size_t last_size=0;

    std::map<std::string,THASH_id > children_hashes_mx;
    std::map<std::string, REF_getter<Cellable>> children_ptrs_mx;

    /// @brief выставляется в конструкторе мутекс не нужен
    unsigned int payload_ctor_idx=hsh::HSH_END;

    /// @brief выставляется в конструкторе, мутекс не нужен
    REF_getter<data_base> data=nullptr;
public:
    /// @brief выставляется и читается в потоке ноды, мутекс не нужен.
    bool is_dirty=false;

    Cellable(Cellable* _parent, const std::string & id):Refcountable("cellable"),  parent(_parent), m_id(id)
    {
    }
    void setDirty()
    {
        is_dirty=true;
        // if(bc.valid())
        //     calcers_Z.insert(bc);
        if(parent)
        {
            parent->setDirty();
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
        {
            MutexLocker lk(mx);
            o<<children_hashes_mx;
            bool valid=data.valid();
            o<<valid;
            if(valid)
                data->pack(o);

        }
    }
    virtual void unpack(inBuffer& in)
    {
        int v=in.get_PN();
        // in>>m_id;
        in>>payload_ctor_idx;
        {
            MutexLocker lk(mx);
            in>>children_hashes_mx;
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


    REF_getter<Cellable> getLeafOrCreate(const std::string& id, IDatabase* db, MutexLockerDeferred &l);
    REF_getter<Cellable> getLeafNoCreate(const std::string& id, IDatabase* db, MutexLockerDeferred &l);

    void calc_tree_hash(_db_to_save &db_dump);

};
// static const char* base16_TABLE[62] = {
//     "0","1","2","3","4","5","6","7","8","9",
//     "A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z",
//     "a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z"
// };
inline void appendRelativeInternalPath(std::vector<std::string>&vs, const std::string & k, int depth2)
{
    MUTEX_INSPECTOR;
    if(k.size()<depth2)
    {
        throw CommonError("if(k.size()<depth2)");
    }
    int i=0;
    for(i=0; i<depth2; i++)
    {
        vs.push_back(k.substr(i, 1));
    }
    if(k.size()>depth2)
    {
        vs.push_back(k.substr(depth2));
    }
}


