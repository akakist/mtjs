

#add_executable(blscc4
# blscc4.cpp
#)
# add_executable(gen_keys
#  gen_keys.cpp
#  base62.cpp
#  mcrypto.cpp
# )
#target_link_libraries(blscc4 bls384_256 mcl)

add_library(bctools STATIC
    tree.cpp
#    messages.cpp
#    mcrypto.cpp
    base62.cpp
    normalizeNick.cpp
root_contract.cpp
tr_exec.cpp
cellable.cpp
execute_transaction.cpp
)


           
