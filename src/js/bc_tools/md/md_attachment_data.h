#pragma once
#include "md_Base.h"
#include "md_TX.h"
#include "bigint.h"

namespace MsgData
{
    struct attachment_data: public Base
    {
        attachment_data():Base(msgid::attachment_data)
        {

        }
        // std::vector<REF_getter<TX>> trs;
        std::pair<int,std::string> block_report={0,""};
        std::map<THASH_id,transaction_report> transaction_reports;
        std::map<std::string,BigInt> fees;
        std::map<NODE_id,BigInt> rewards;
        std::vector<std::string> emitted_events;

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            b<<block_report<<transaction_reports<<fees<<rewards<<emitted_events;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>block_report>>transaction_reports>>fees>>rewards>>emitted_events;
        }

        // void clear()
        // {
        //     trs.clear();
        //     transaction_reports.clear();
        //     fees.clear();
        //     rewards.clear();
        // }
        void update(Blake2bHasher &h) const
        {
            // for(auto &z:trs)
            // {
            //     z->update(h);
            // }
            h.update(std::to_string(block_report.first));
            h.update(block_report.second);
            for(auto &z: transaction_reports)
            {
                z.second.update(h);
            }
            for(auto &z: fees)
            {
                h.update(z.first);
                h.update(z.second.toString());
            }
            for(auto &z: rewards)
            {
                h.update(z.first.container);
                h.update(z.second.toString());
            }
            for(const auto &z: emitted_events)
            {
                h.update(z);
            }
        }
    };


}
// inline outBuffer& operator<< (outBuffer& b,const REF_getter<MsgData::attachment_data> &s)
// {
//     s->pack(b);
//     return b;
// }
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::attachment_data> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::attachment_data> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::attachment_data();
    s->unpack2(b);
    return b;
}


