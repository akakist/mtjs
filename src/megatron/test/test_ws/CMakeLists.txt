
include_directories(../../lib/miniupnp-master)

add_executable(test-wsc
main_.cpp
mainTestWS.cpp
testWebServerService2.cpp
testWSCService.cpp

)

target_link_libraries(test-wsc  megatron-main   zsingle megatron-core  ${OPENSSL_LIBRARIES}  z pthread dl)




