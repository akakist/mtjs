#pragma once
#include "md_Base.h"
#include "md_ValidateBlockRSP.h"
namespace MsgData
{
    // Ответ на GetValidateBlockRSPREQ: собственный подписанный ValidateBlockRSP
    // ответчика за запрошенную эпоху (или пустой вектор, если замка нет).
    // ValidateBlockRSP несёт и тело блока (diffs/deleted/tx_hashes/att_data),
    // поэтому новый лидер получает тело даже если не получал ValidateBlockREQ.
    struct GetValidateBlockRSPRSP: public Base
    {
        GetValidateBlockRSPRSP():Base(msgid::GetValidateBlockRSPRSP)
        {
        }
        static Base* construct()
        {
            return new GetValidateBlockRSPRSP();
        }
        int64_t epoch=0;
        std::vector<REF_getter<ValidateBlockRSP>> rsps;

        void update(Blake2bHasher& h) const
        {
            MUTEX_INSPECTOR;
            h.update(std::to_string(epoch));
            for(auto& z:rsps)
                z->update(h);
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            b<<epoch;
            b<<rsps;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>epoch;
            b>>rsps;
        }
    };
}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::GetValidateBlockRSPRSP> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::GetValidateBlockRSPRSP> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::GetValidateBlockRSPRSP();
    s->unpack2(b);
    return b;
}
