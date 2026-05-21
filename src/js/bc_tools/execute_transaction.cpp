#include "execute_transaction.h"
#include "REF.h"
#include "msg.h"
#include "msg_tx.h"
#include "t_params.h"
#include "tr_exec.h"
#include "md/md_TxMint.h"
#include <vector>
void execute_transaction(const THASH_id& tx_id, t_params &t, const std::string &senderAddress, const REF_getter<MsgData::TX> &tx, const REF_getter<fee_calcer> &by)
{
    for (int ii = 0; ii < tx->instructions->instructions.size(); ii++)
    {
        // auto h=blake2b_hash(payloads[ii]);
        // inBuffer i2(payloads[ii]);
        // int ty2 = i2.get_PN();
        auto &ins = tx->instructions->instructions[ii];
        switch (ins->type)
        {
        case msgid::TxMint:
        {
            MsgData::TxMint *m=dynamic_cast<MsgData::TxMint*>(tx.get());
            if(!m) throw CommonError("if(!m) 334455");
            // tx::mint mint;
            // mint.unpack(i2);
            auto err = TR::execute(m, t, senderAddress, by, tx_id, ii);
            if (err)
            {
                t.setError(tx_id, ii, *err);
            }
        }
        break;
        default:
            logErr2("unhndled ty2 %s", txName(tx->type));
            t.logError(tx_id, ii, "unhandled transaction type %s", txName(tx->type));
        }
    }
}
