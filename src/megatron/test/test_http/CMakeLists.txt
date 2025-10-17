
include_directories(../../lib/miniupnp-master)

add_executable(test-http 
mainTestHTTP.cpp
main_.cpp
testWebServerService.cpp

)

target_link_libraries(test-http megatron-main   zsingle megatron-core  ${OPENSSL_LIBRARIES}  z pthread dl)




