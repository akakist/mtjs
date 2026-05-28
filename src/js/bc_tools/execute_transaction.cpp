#include "execute_transaction.h"
#include "REF.h"
#include "msg.h"
#include "msg_tx.h"
#include "t_params.h"
#include "tr_exec.h"
#include "md/md_TxMint.h"
#include <vector>
void execute_transaction(const THASH_id& tx_id, t_params &t, const std::string &senderAddress, const nlohmann::json& tx_cmds, const REF_getter<fee_calcer> &by)
{
    logErr2("execute_transaction");
    for (int ii = 0; ii < tx_cmds.size(); ii++)
    {
        // auto h=blake2b_hash(payloads[ii]);
        // inBuffer i2(payloads[ii]);
        // int ty2 = i2.get_PN();

        auto &cmd = tx_cmds[ii];
        auto contract=cmd["contract"].get<std::string>();
        auto method=cmd["method"].get<std::string>();
        auto params=cmd["params"];
        logErr2("contract %s",contract.c_str());
        logErr2("method %s",method.c_str());
        if(contract=="")
        {
            if(method=="mint")
            {
                auto err=TR::execute_mint(params,t,senderAddress,by,tx_id,ii);
                // MsgData::TxMint m;
                // m.user_pk_ed=senderAddress;
                // m.amount=params["amount"].get<std::string>();
                // auto err = TR::execute(&m, t, senderAddress, by, tx_id, ii);
                if (err)
                {
                    t.setError(tx_id, ii, *err);
                }
            }
            else
            {
                t.logError(tx_id, ii, "unhandled method %s for root contract", method.c_str());
            }
        }
#ifdef KALL        
        switch (ins["type"].get<int>())
        {
        case msgid::TxMint:
        {
            MsgData::TxMint *m=dynamic_cast<MsgData::TxMint*>(ins.get());
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
#endif
    }
}
