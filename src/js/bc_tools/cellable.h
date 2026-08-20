#pragma once
#include "REF.h"
#include <string>
#include "commonError.h"
#include "IDatabase.h"
#include <vector>
#include "db_to_save.h"
#include "hsh.h"
#include "THASH_id.h"

struct Cellable;

struct Rollback
{
    std::map<Cellable*, std::string> data;
    size_t size()
    {
        size_t ret;
        for(auto &z: data)
            ret+=z.second.size();
        return ret;
    }
};
struct data_base : public Refcountable
{
    int type;
    Cellable *parent;

    time_t create_time=0;
    int ttl=-1;

    // uint64_t last_update_epoch;

    data_base(int t, Cellable* _parent, time_t _create_time, int _ttl ): Refcountable("data_base"),
        type(t), parent(_parent), create_time(_create_time),ttl(_ttl) {
            // last_update_epoch=0;
        }
    ~data_base()
    {
    }
    void setDirty(Rollback* roll);
;
    virtual void pack(outBuffer& o) const
    {
        o<<1;
        o<<type;
        o<<create_time<<ttl;
        // o<<last_update_epoch;
    }
    virtual void unpack(inBuffer& in)
    {
        int ver=in.get_PN();
        in>>type;
        in>>create_time>>ttl;
        // in>>last_update_epoch;
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
    // size_t last_size=0;

    std::map<std::string,THASH_id > children_hashes_mx;
    std::map<std::string, REF_getter<Cellable>> children_ptrs_mx;

    /// @brief выставляется в конструкторе мутекс не нужен
    unsigned int payload_ctor_idx=hsh::HSH_END;

    /// @brief выставляется в конструкторе, мутекс не нужен
    REF_getter<data_base> data=nullptr;
public:
    /// @brief выставляется и читается в потоке ноды, мутекс не нужен.
    bool is_dirty=false;
    // uint64_t last_update_epoch;

    Cellable(Cellable* _parent, const std::string & id):Refcountable("cellable"),  parent(_parent), m_id(id)
    {
    }
    void setDirty__(Rollback* r)
    {
    MUTEX_INSPECTOR;
        {
    MUTEX_INSPECTOR;
            M_LOCK(mx);
            if(r)
            {
    MUTEX_INSPECTOR;
                if(!r->data.count(this))
                    r->data[this]=getBuffer_mx();
            }
            is_dirty=true;
            // last_update_epoch=epoch;

        }
        // if(bc.valid())
        //     calcers_Z.insert(bc);
        if(parent)
        {
            parent->setDirty__(r);
        }
    }
    std::string dump();

    REF_getter<Cellable> getLeafOrCreate(const std::string &id, IDatabase *db, MutexLockerDeferred &l, Rollback *roll);

    std::string getDbId() const;

    void get_path(std::vector<const Cellable*> &s) const
    {
        s.push_back(this);
        if(parent)
        {
            parent->get_path(s);
        }
    }
    void get_path(std::deque<std::string> &s) const
    {
        if(parent)
        {
            parent->get_path(s);
        }
        s.push_back(this->m_id);
    }
    virtual void pack_mx(outBuffer& o)
    {
        o<<1;
        // o<<m_id;
        o<<payload_ctor_idx;
        // o<<last_update_epoch;
        {
            // MutexLocker lk(mx);
            o<<children_hashes_mx;
            bool valid=data.valid();
            o<<valid;
            if(valid)
                data->pack(o);

        }
    }
    virtual void unpack_mx(inBuffer& in)
    {
        int v=in.get_PN();
        // in>>m_id;
        in>>payload_ctor_idx;
        // in>>last_update_epoch;
        {
            // MutexLocker lk(mx);
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
    std::string getBuffer_mx()
    {
        outBuffer o;
        pack_mx(o);
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


    // REF_getter<Cellable> getLeafOrCreate(const std::string& id, IDatabase* db, MutexLockerDeferred &l);
    REF_getter<Cellable> getLeafNoCreate(const std::string& id, IDatabase* db, MutexLockerDeferred &l);

    void calc_tree_hash(_db_to_save &db_dump);
};
// static const char* base16_TABLE[62] = {
//     "0","1","2","3","4","5","6","7","8","9",
//     "A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z",
//     "a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z"
// };
// inline void appendRelativeInternalPath(std::vector<std::string>&vs, const std::string & k, int depth2)
// {
//     MUTEX_INSPECTOR;
//     if(k.size()<depth2)
//     {
//         throw CommonError("if(k.size()<depth2)");
//     }
//     int i=0;
//     for(i=0; i<depth2; i++)
//     {
//         vs.push_back(k.substr(i, 1));
//     }
//     if(k.size()>depth2)
//     {
//         vs.push_back(k.substr(depth2));
//     }
// }


