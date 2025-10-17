include_directories(Event) 



add_executable(test-referrer-client

appHandler.cpp
mainTestReferrerClient.cpp
main_.cpp


)

target_link_libraries(test-referrer-client  megatron-main   zsingle megatron-core  ${OPENSSL_LIBRARIES}   z pthread dl)


