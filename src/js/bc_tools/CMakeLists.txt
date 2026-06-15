

#add_executable(blscc4
# blscc4.cpp
#)
# add_executable(gen_keys
#  gen_keys.cpp
#  base16.cpp
#  mcrypto.cpp
# )
#target_link_libraries(blscc4 bls384_256 mcl)

add_library(bctools STATIC
    tree.cpp
#    messages.cpp
#    mcrypto.cpp
    base16.cpp
    normalizeNick.cpp
root_contract.cpp
tr_exec.cpp
cellable.cpp
execute_transaction.cpp
msg.cpp
msgFactory.cpp
init_root.cpp
base16.h
blake2bHasher.h
BLOCK_id.h
blst_cp.h
CDatabase.h
cellable.h
db_to_save.h
dbHistory.h
dirty.h
execute_transaction.h
fee_calcer.h
getenv2.h
hsh.h
IDatabase.h
init_root.h
msg.h
msgFactory.h
NODE_id.h
promiseCapability.h
QUORUM.h
root_contract.h
s_ed.h
t_params.h
THASH_id.h
tr_exec.h
TRANSACTION_id.h
tree.h
TypedStr.h
md/md_LcEnvelopeREQ.h
get_tariff.h

)


           
