#include "execute_transaction.h"
#include "REF.h"
#include "tr_exec.h"
// #include "msg_tx.h"
#include "t_params.h"
#include <optional>
void execute_transaction(const THASH_id &tx_id, t_params &t, const ADDRESS_id &senderAddress, const std::string &tx_cmds, const REF_getter<fee_calcer> &by)
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
                t.emit_command(tx_id, ii, "error", 
                    R"({"code":-32602,"error":"contract, method, params fields required"})");
                err=true;
            }
            // throw CommonError("if(!contract.isString() || !method.isString() || !params.isObject())");
            if (!err && contract == "root")
            {
                std::optional<std::string> err;
                auto meth=method.toString();
                // logErr2("method %s",meth.c_str());
                if (meth == "mint")
                    err = TR::execute_mint(params, t, senderAddress, by, tx_id, ii);
                else if (meth == "transfer")
                    err = TR::execute_transfer(params, t, senderAddress, by, tx_id, ii);
                else if (meth == "node_create")
                    err = TR::execute_node_create(params, t, senderAddress, by, tx_id, ii);
                else if (meth == "node_update")
                    err = TR::execute_node_update(params, t, senderAddress, by, tx_id, ii);
                else if (meth == "node_stake")
                    err = TR::execute_node_stake(params, t, senderAddress, by, tx_id, ii);
                else if (meth == "node_unstake")
                    err = TR::execute_unstake_node(params, t, senderAddress, by, tx_id, ii);
                else if (meth == "node_enable")
                    err = TR::execute_node_enable(params, t, senderAddress, by, tx_id, ii);
                else if (meth == "contract_deploy")
                    err = TR::execute_contract_deploy(params, t, senderAddress, by, tx_id, ii);
                else if (meth == "contract_update")
                    err = TR::execute_contract_update(params, t, senderAddress, by, tx_id, ii);
                else
                {
                    t.emit_command(tx_id, ii, "error", 
                        R"({"error":"unhandled method %s for root contract"})", 
                        method.toString().c_str());
                }
                if (err)
                {
                    t.emit_command(tx_id, ii, "error", 
                        R"({"error":"%s"})", 
                        err->c_str());                
                }

            }
            if(!err)
            {
                /// exec js contract
                
            }

        }
    }
    else
    {
        t.emit_block("error", R"({"code":-32602,"error":"tx body must be array"})");
        // t.att_data->block_report= {1,"tx body must be array"};
    }
}
