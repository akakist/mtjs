#pragma once
#include "cellable.h"
#include "NODE_id.h"
struct bc_nodelist:  public data_base
{

    bc_nodelist(Cellable *p):data_base(hsh::bc_nodelist,p, 0,-1) {}
    private:
    std::set<NODE_id> list;
    public:
    std::set<NODE_id> getList()
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
