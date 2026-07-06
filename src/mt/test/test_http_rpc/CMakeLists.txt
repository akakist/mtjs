
include_directories( .) 


ADD_SUBDIRECTORY(server)
ADD_SUBDIRECTORY(serverWeb)

add_executable(test-http-rpc
mainTestHTTP_RPC.cpp
main_.cpp

)

target_link_libraries(test-http-rpc prodtestServer prodtestWebServer  megatron-main   zsingle megatron-core  ${OPENSSL_LIBRARIES}  z pthread dl)


