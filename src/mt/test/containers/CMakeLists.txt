


add_executable(test-containers1 
containers_test_service.cpp
main.cpp

)
target_link_libraries(test-containers1 megatron-main megatron-core zsingle  ${OPENSSL_LIBRARIES}   z pthread ${DL})



add_test(NAME test-containers1
         COMMAND test-containers1)


