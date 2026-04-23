#include "execute_transaction.h"
#include "msg.h"
#include "msg_tx.h"
#include "tr_exec.h"
void
execute_transaction(int tx_id, t_params& t, const std::string& senderNick, const std::vector<std::string>& payloads,const REF_getter<fee_calcer> &by)
{
    for(int ii=0; ii<payloads.size(); ii++)
    {
        inBuffer i2(payloads[ii]);
        int ty2=i2.get_PN();
        switch(ty2)
        {
        case tx_id::mint:
        {
            tx::mint mint;
            mint.unpack(i2);
            auto err=TR::execute(mint,t,senderNick, by, tx_id,ii);
            if(err)
            {
                t.setError(tx_id,ii,*err);
            }

        }
        break;
        case tx_id::registerUser:
        {
            tx::registerUser rn;
            rn.unpack(i2);
            auto err=TR::execute(rn,t,senderNick, by, tx_id,ii);
            if(err)
            {
                t.setError(tx_id,ii,*err);
            }

        }
        break;
        default:
            logErr2("unhndled ty2 %s",txName(ty2));
            t.logError(tx_id,ii,"unhandled transaction type %s",txName(ty2));
        }

    }

}
