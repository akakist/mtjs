
add_executable(mt-test-${PORT}.exe 
mainTest.cpp
)
include_directories(../../main)
IF(FreeBSD)
target_link_libraries(mt-test-${PORT}.exe megatron-main zsingle megatron-core  ${OPENSSL_LIBRARIES}  z  pthread   )
ELSE()
target_link_libraries(mt-test-${PORT}.exe megatron-main zsingle megatron-core  ${OPENSSL_LIBRARIES}  z  pthread   dl  )
ENDIF()


install(TARGETS mt-test-${PORT}.exe DESTINATION ${BIN_TARGET_DIR})

