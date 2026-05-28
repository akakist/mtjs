#include "execute_transaction.h"
#include "REF.h"
#include "msg.h"
#include "msg_tx.h"
#include "t_params.h"
#include "tr_exec.h"
#include <vector>
void execute_transaction(const THASH_id& tx_id, t_params &t, const std::string &senderAddress, const nlohmann::json& tx_cmds, const REF_getter<fee_calcer> &by)
{
    for (int ii = 0; ii < tx_cmds.size(); ii++)
    {

        auto &cmd = tx_cmds[ii];
        auto contract=cmd["contract"].get<std::string>();
        auto method=cmd["method"].get<std::string>();
        auto params=cmd["params"];
        if(contract=="")
        {
            if(method=="mint")
            {
                auto err=TR::execute_mint(params,t,senderAddress,by,tx_id,ii);
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
    }
}
