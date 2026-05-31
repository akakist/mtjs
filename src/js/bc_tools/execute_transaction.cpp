#include "execute_transaction.h"
#include "REF.h"
#include "msg.h"
// #include "msg_tx.h"
#include "t_params.h"
#include "tr_exec.h"
#include <optional>
#include <vector>
void execute_transaction(const THASH_id &tx_id, t_params &t, const std::string &senderAddress, const std::string &tx_cmds, const REF_getter<fee_calcer> &by)
{
    yyjson::Document doc(tx_cmds);
    yyjson::Value root = doc.root(); 

    if(root.isArray())
    {
        for (int ii=0; ii < root.size(); ii++)    
        {
            bool err=false;
            yyjson::Value item = root[ii];
            auto contract = item/"contract";
            auto method = item/"method";
            auto params= item/"params";
            if(!contract.isString() || !method.isString() || !params.isObject())
            { 
                t.setError(tx_id,ii,"contract method params fields required");
                err=true;
            }
                // throw CommonError("if(!contract.isString() || !method.isString() || !params.isObject())");
            if (!err && contract == "")
            {
                std::optional<std::string> err;
                auto meth=method.toString();
                // logErr2("method %s",meth.c_str());
                if (meth == "mint")
                    err = TR::execute_mint(params, t, senderAddress, by, tx_id, ii);
                else if (meth == "transfer")
                    err = TR::execute_transfer(params, t, senderAddress, by, tx_id, ii);
                else if (meth == "create_node")
                    err = TR::execute_create_node(params, t, senderAddress, by, tx_id, ii);
                else if (meth == "update_node")
                    err = TR::execute_update_node(params, t, senderAddress, by, tx_id, ii);
                else
                {
                    t.logError(tx_id, ii, "unhandled method %s for root contract", method.toString().c_str());
                }
                if (err)
                {
                    t.setError(tx_id, ii, *err);
                }

            }

        }
    }
    else 
    {
        t.att_data->block_report={1,"tx body must be array"};
    }
}
