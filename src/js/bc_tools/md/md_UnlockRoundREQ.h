#pragma once
#include "md_Base.h"
#include "md_ValidateBlockRSP.h"
namespace MsgData
{
    // Шаг 5: доказательство сплита. Лидер, собравший >=2/3 голосов, но ни
    // одного root с 2/3, рассылает все подписи — каждый проверяет, что кворум
    // голосов есть, а кворума за один root нет, и снимает свой замок.
    struct UnlockRoundREQ: public Base
    {
        UnlockRoundREQ():Base(msgid::UnlockRoundREQ)
        {
        }
        static Base* construct()
        {
            return new UnlockRoundREQ();
        }
        int64_t epoch=0;
        THASH_id base_root;
        std::vector<REF_getter<ValidateBlockRSP>> proof;

        void update(Blake2bHasher& h) const
        {
            MUTEX_INSPECTOR;
            h.update(std::to_string(epoch));
            h.update(base_root.container);
            for (auto& z : proof)
                z->update(h);
        }
        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            b<<epoch;
            b<<base_root;
            b<<proof;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b>>epoch;
            b>>base_root;
            b>>proof;
        }
    };
}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::UnlockRoundREQ> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::UnlockRoundREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::UnlockRoundREQ();
    s->unpack2(b);
    return b;
}
