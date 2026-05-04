#include "msg.h"
        MsgEvent::BlockAcceptedREQ::BlockAcceptedREQ()
        :Base(msgid::BlockAcceptedREQ), leader_certificateZ(new LeaderCertificate()), block_payload(new BlockInfo)
        {

        }

        void MsgEvent::BlockAcceptedREQ::pack(outBuffer& b) const 
        {
            MUTEX_INSPECTOR;
            Base::pack(b);
            leader_certificateZ->pack(b);
            block_payload->pack(b);
            b<<node_validators<<agg_sig;
        }
        void MsgEvent::BlockAcceptedREQ::unpack(inBuffer& b) 
        {
            MUTEX_INSPECTOR;
            Base::unpack(b);
            leader_certificateZ->unpack2(b);
            block_payload->unpack2(b);
            b>>node_validators>>agg_sig;
        }
