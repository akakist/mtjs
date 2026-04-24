

#add_executable(blscc4
# blscc4.cpp
#)
 add_executable(keygen
  keygen.cpp
    ../bc_tools/base62.cpp
#  base62.cpp
#  mcrypto.cpp
 )
 add_executable(keygen2
  keygen2.cpp
#  base62.cpp
#  mcrypto.cpp
 )
target_link_libraries(keygen blst crypto sodium)
target_link_libraries(keygen2 blst crypto sodium)
# add_executable(client
  # client.cpp
#  base62.cpp
#  mcrypto.cpp
# )
#target_link_libraries(client bctools  мегатронmegatron-core bls384_256 mcl crypto sodium gmp)

#add_library(bctools STATIC
#    tree.cpp
#    messages.cpp
#    mcrypto.cpp
#    base62.cpp
#    normalizeNick.cpp
#)


           
