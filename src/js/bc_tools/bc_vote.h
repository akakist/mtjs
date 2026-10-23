#pragma once
#include "cellable.h"
// #include
struct vote
{
    std::string param;
    uint64_t value;
    std::set<NODE_id> nodes;
    time_t create_time;
    int64_t ttl = 0;                     // <-- новое
};
inline outBuffer& operator<<(outBuffer &o,const vote& z)
{
    o<<z.param<<z.value<<z.nodes<<z.create_time<<z.ttl;   // <-- ttl в конце
    return o;
}
inline inBuffer& operator>>(inBuffer &o,vote& z)
{
    o>>z.param>>z.value>>z.nodes>>z.create_time>>z.ttl;   // <-- ttl в конце
    return o;
}
struct bc_votes:  public data_base
{

    bc_votes(Cellable *p):data_base(hsh::bc_votes,p, 0,-1) {}
    std::map<THASH_id, vote> votes;
    size_t size() const
    {

        return  0;
    }
    void pack(outBuffer&b) const final
    {
        data_base::pack(b);
        b<<1;
        b<<votes;

    }
    void unpack(inBuffer&b) final
    {
        data_base::unpack(b);
        auto v=b.get_PN();
        b>>votes;

    }
};
