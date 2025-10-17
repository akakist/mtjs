
include_directories(../../lib/miniupnp-master)

add_executable(test-rpc2 
mainTest.cpp
main_.cpp
rpcTestService1.cpp
rpcTestService2.cpp

)

target_link_libraries(test-rpc2 megatron-main   zsingle megatron-core  ${OPENSSL_LIBRARIES}   z pthread dl)




