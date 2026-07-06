

add_executable(test-timer 
___testEvent.h
mainTestTimer.cpp
main_.cpp
testTimerService.cpp
testTimerService.h


)

target_link_libraries(test-timer megatron-main   zsingle megatron-core  ${OPENSSL_LIBRARIES}  z pthread dl)


add_test(NAME test-timer
         COMMAND test-timer)


