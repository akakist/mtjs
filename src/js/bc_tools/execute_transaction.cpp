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
            yyjson::Value item = root[ii];
            auto contract = item/"contract";
            auto method = item/"method";
            auto params= item/"params";
            if(!contract.isString() || !method.isString() || !params.isObject())
                throw CommonError("if(!contract.isString() || !method.isString() || !params.isObject())");
            if (contract == "")
            {
                std::optional<std::string> err;
                if (method == "mint")
                    err = TR::execute_mint(params, t, senderAddress, by, tx_id, ii);
                else if (method == "transfer")
                    err = TR::execute_transfer(params, t, senderAddress, by, tx_id, ii);
                else if (method == "create_node")
                    err = TR::execute_create_node(params, t, senderAddress, by, tx_id, ii);
                else if (method == "update_node")
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
    else throw CommonError("!if(root.isArray())");
    // for (auto ii=0; ii < roottx_cmds.size(); ii++)
    // {

    //     auto &cmd = tx_cmds[ii];
    //     auto contract = cmd["contract"].get<std::string>();
    //     auto method = cmd["method"].get<std::string>();
    //     auto params = cmd["params"];
    //     if (contract == "")
    //     {
    //         std::optional<std::string> err;
    //         if (method == "mint")
    //             err = TR::execute_mint(params, t, senderAddress, by, tx_id, ii);
    //         else if (method == "transfer")
    //             err = TR::execute_transfer(params, t, senderAddress, by, tx_id, ii);
    //         else if (method == "create_node")
    //             err = TR::execute_create_node(params, t, senderAddress, by, tx_id, ii);
    //         else if (method == "update_node")
    //             err = TR::execute_update_node(params, t, senderAddress, by, tx_id, ii);
    //         else
    //         {
    //             t.logError(tx_id, ii, "unhandled method %s for root contract", method.c_str());
    //         }
    //         if (err)
    //         {
    //             t.setError(tx_id, ii, *err);
    //         }

    //     }
    // }
}
