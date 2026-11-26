#pragma once
#include "md_Base.h"
#include "md_HeartBeatREQ.h"
#include "md_LeaderCert.h"
#include "md_SlashCmd.h"
namespace MsgData
{
    /// Тип оффера (субблока): обычный набор tx либо детерминированный слэш.
    enum SubBlockType : int { SUBBLOCK_TX = 0, SUBBLOCK_SLASH = 1 };

    struct ValidateBlockREQ: public Base
    {

        ValidateBlockREQ():Base(msgid::ValidateBlockREQ)
        {
        }
        static Base* construct()
        {
            return new ValidateBlockREQ();
        }

        int subblock_type = SUBBLOCK_TX;
        // REF_getter<MsgData::HeartBeatREQ>  heart_beat;
        std::vector<REF_getter<MsgData::TX> > transaction_bodies;
        /// эвиденс двойной подписи (только для SUBBLOCK_SLASH)
        std::vector<REF_getter<MsgData::SlashCmd> > slashes;
        // TIME_mks block_timestamp;
        void update(Blake2bHasher& h) const
        {
            h.update(std::to_string(subblock_type));
            for(auto& z: transaction_bodies)
            {
                
                h.update(z->tx_body);
            }
            for(auto& z: slashes)
            {
                z->update(h);
            }
        }

        void pack(outBuffer& b) const final
        {
            MUTEX_INSPECTOR;

            Base::pack(b);
            b << subblock_type << transaction_bodies << slashes;
        }
        void unpack(inBuffer& b) final
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            b >> subblock_type >> transaction_bodies >> slashes;
        }

    };

}
inline outBuffer & operator<< (outBuffer& b,const REF_getter<MsgData::ValidateBlockREQ> &s)
{
    b<<1;
    s->pack(b);
    return b;
}
inline inBuffer & operator>> (inBuffer& b,  REF_getter<MsgData::ValidateBlockREQ> &s)
{
    auto ver=b.get_PN();
    if(!s.valid())
        s=new MsgData::ValidateBlockREQ();
    s->unpack2(b);
    return b;
}
