#pragma once
#include "cellable.h"
#include "NODE_id.h"
struct bc_nodelist:  public data_base
{

    bc_nodelist(Cellable *p):data_base(hsh::bc_nodelist,p, 0,-1) {}
    private:
    std::set<NODE_id> list;
    public:
    size_t size() const 
    {
        M_LOCK(parent->mx);
        size_t sz=size_();
        for(auto &z: list)
            sz+=z.container.size();
        return sz;
    }
    std::set<NODE_id> getList() const
    {
        M_LOCK(parent->mx);
        return list;
    }
    int count(const NODE_id& n)
    {
        M_LOCK(parent->mx);
        return list.count(n);
    }
    void insert(const NODE_id& n)
    {
        M_LOCK(parent->mx);
        list.insert(n);
    }
    void pack(outBuffer&b) const final
    {
        data_base::pack(b);
        b<<1;
        b<<list;

    }
    void unpack(inBuffer&b) final
    {
        data_base::unpack(b);
        auto v=b.get_PN();
        b>>list;

    }
};
