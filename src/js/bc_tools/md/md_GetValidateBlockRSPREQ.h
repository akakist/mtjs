#pragma once
#include "md_Base.h"
#include "THASH_id.h"
namespace MsgData
{
    // Шаг 3 смены раунда: новый лидер просит у комитета подписи за высоту epoch.
    // Запрос именно по ЭПОХЕ, а не только по root: при расколе комитет мог
    // подписать разные руты на одной высоте, и для доказательства сплита
    // (шаг 5) лидеру нужны все голоса, а не только за свой root.
    // root_hint — подсказка «какой root меня интересует» (для логов/фильтра),
    // ответчик возвращает свой фактический замок на этой эпохе.
    struct GetValidateBlockRSPREQ: public Base
    {
        GetValidateBlockRSPREQ():Base(msgid::GetValidateBlockRSPREQ)
        {
        }
        static Base* construct()
        {
            return new GetValidateBlockRSPREQ();
        }
        int64_t epoch=0;      // высота (hb->new_epoch), за которую нужны подписи
        THASH_id base_root;   // prev_root раунда (пустой = не проверять)
        THASH_id root_hint;   // желаемый root (пустой = любой)

        void update(Blake2bHasher& h) const
        {
            h.update(std::to_string(epoch));
            h.update(base_root.container);
            h.update(root_hint.container);
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            b<<epoch;
            b<<base_root;
            b<<root_hint;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>epoch;
            b>>base_root;
            b>>root_hint;
        }
    };
}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::GetValidateBlockRSPREQ> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::GetValidateBlockRSPREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::GetValidateBlockRSPREQ();
    s->unpack2(b);
    return b;
}
