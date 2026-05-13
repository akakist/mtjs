#include "msg.h"
#include "commonError.h"
#include "mutexable.h"
#include <map>
#include <string>

Mutex mx_alloc;
std::map<std::string, int> m_allocs;
void inc_alloc(const std::string &s)
{
    M_LOCK(mx_alloc);
    auto it=m_allocs.find(s);
    if(it==m_allocs.end())
    {
        m_allocs[s]=1;    
    }
    else
    it->second++;
}
void dec_alloc(const std::string &s)
{
    M_LOCK(mx_alloc);
    auto it=m_allocs.find(s);
    if(it==m_allocs.end())
    {
        m_allocs[s]=1;    
    }
    else
    it->second--;
}
std::string getAllocsInfo()
{
    M_LOCK(mx_alloc);
    std::string o;
    for (auto &z : m_allocs)
    {
        if(z.second)
        o += z.first + " " + std::to_string(z.second) + "\n";

    }
    return o;
}

MsgEvt::BlockAcceptedREQ::BlockAcceptedREQ()
    : Base(msgid::BlockAcceptedREQ), leader_certificateZ(new LeaderCertificate()), block_payload(new BlockInfo)
{
    INC_ALLOC("BlockAcceptedREQ");
}

void MsgEvt::BlockAcceptedREQ::pack(outBuffer &b) const
{
    XTRY;
    MUTEX_INSPECTOR;
    Base::pack(b);
    leader_certificateZ->pack(b);
    block_payload->pack(b);
    b << node_validators << agg_sig;
    XPASS;
}
void MsgEvt::BlockAcceptedREQ::unpack(inBuffer &b)
{
    XTRY;
    MUTEX_INSPECTOR;
    Base::unpack(b);
    leader_certificateZ->unpack2(b);
    block_payload->unpack2(b);
    b >> node_validators >> agg_sig;
    XPASS;
}
