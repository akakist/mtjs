#pragma once
#include "REF.h"
#include <string>
#include "commonError.h"
#include <vector>
#include "db_to_save.h"
#include "hsh.h"
#include "THASH_id.h"
#include "Rollback.h"
struct Cellable;

struct data_base : public Refcountable
{
    int type;
    Cellable *parent;

    std::string getDbId() const;



    data_base(int t, Cellable* _parent ): Refcountable("data_base"),
        type(t), parent(_parent) {
    }
    ~data_base()
    {
    }
    void snapshot(Rollback* roll);
    ;
    size_t size_() const
    {
        return sizeof(type);
    }
    virtual size_t size() const=0;
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
    /// @brief  не изменяется, выставляется только в конструкторе
    Cellable * parent=nullptr;

    /// @brief  не изменяется, выставляется только в конструкторе
    const std::string m_id;

    TIME_mks last_block_timestamp;   // момент последнего изменения ячейки

    std::map<std::string,THASH_id > children_hashes;
    std::map<std::string, REF_getter<Cellable>> children_ptrs;

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
    size_t size()
    {
        size_t sz=0;
        sz+=m_id.size();
        for(auto& z:children_hashes)
        {
            sz+=z.first.size();
            sz+=z.second.container.size();
        }
        return sz;
    }
    // 1) Снапшот для отката — СТРОГО до мутации. Идемпотентно (первый вызов).
    void snapshot(Rollback* r, const std::set<REF_getter<Cellable>>& exclude)
    {
        if (r && !r->data.count(this) && !exclude.count(this))
            r->data[this] = getBuffer_mx();
        if(parent)
            parent->snapshot(r,exclude);
    }
    // 2) Пометить dirty + поднять ts + прокинуть родителям. Рекурсия идёт через
    //    setDirty__ — иначе у предков не снимется снапшот, а их children_hashes
    //    тоже меняются при пересчёте и должны откатываться.
    void markDirty(TIME_mks block_timestamp)
    {
        is_dirty = true;
        if (block_timestamp > last_block_timestamp)
            last_block_timestamp = block_timestamp;
        if (parent)
            parent->markDirty(block_timestamp);
    }

    std::string getDbId() const;
    void clear(const std::string &id)
    {
        std::deque<std::string> v;
        if(id.size()>0)
            v.push_back(id.substr(0,1));
        if(id.size()>1)
            v.push_back(id.substr(1,1));
        if(id.size()>2)
            v.push_back(id.substr(2,1));
        if(id.size()>3)
            v.push_back(id.substr(3,1));
        if(id.size()>4)
            v.push_back(id.substr(4,28));

        Cellable *r=this;
        while(v.size())
        {
            auto it=r->children_ptrs.find(v[0]);
            if(it!=r->children_ptrs.end())
            {
                r=it->second.get();
                v.pop_front();
            }
            else break;
        }
        if(v.size() == 0)
        {
            auto id=r->m_id;
            r=r->parent;
            if(r)
            {
                r->children_ptrs.erase(id);
            }
        }

    }

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
        o<<payload_ctor_idx;
        o<<last_block_timestamp;
        {
            o<<children_hashes;
            bool valid=data.valid();
            o<<valid;
            if(valid)
                data->pack(o);

        }
    }
    virtual void unpack_mx(inBuffer& in)
    {
        int v=in.get_PN();
        (void)v;   // версии не используем: пока нет прода, формат один
        in>>payload_ctor_idx;
        in>>last_block_timestamp;
        {
            in>>children_hashes;
            bool valid;
            in>>valid;
            // Тумбстоун/пустая ячейка: данных нет. Чистим data, иначе у ячейки,
            // которая раньше была гранулой, останется старый объект и буфер разойдётся.
            if(!valid) data = nullptr;
            if(valid)
            {
                if(payload_ctor_idx<hsh::HSH_END)
                {
                    data=db_constructors[payload_ctor_idx](this);
                    if(data.get())
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



    void calc_tree_hash(_db_to_save &db_dump);
    void get_dirty_leaves(_db_to_save &db_dump);

};

// RAII guard для записи в ячейку: конструктор делает snapshot (если не новая),
// деструктор — markDirty. Через const-геттер писать нельзя, поэтому единственный
// путь к мутации идёт через guard и снапшот не забывается.
template <class T>
class WriteGuard
{
    REF_getter<T> keep;   // держим data_base живым, пока guard жив
    T* p;
    Rollback* roll;
    TIME_mks ts;
public:
    WriteGuard(const REF_getter<T>& x, Rollback* r, TIME_mks t, bool is_new = false)
        : keep(x), p(keep.get()), roll(r), ts(t)
    {
        if (!is_new && roll)
            p->snapshot(roll);   // data_base::snapshot -> parent->snapshot
    }
    T* operator->() { return p; }
    T& operator*() { return *p; }
    ~WriteGuard() { p->parent->markDirty(ts); }
    WriteGuard(const WriteGuard&) = delete;
    WriteGuard& operator=(const WriteGuard&) = delete;
};


void deleteGranule(data_base* g, Rollback* roll,_db_to_save &db_dump, TIME_mks block_timestamp);
