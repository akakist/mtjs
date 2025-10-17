
add_executable(mt-test-${PORT}.exe 
mainTest.cpp
)
include_directories(../../main)
IF(FreeBSD)
target_link_libraries(mt-test-${PORT}.exe megatron-main zsingle megatron-core  ${OPENSSL_LIBRARIES}  z  pthread sqlitecpp sqlite3 )
ELSE()
target_link_libraries(mt-test-${PORT}.exe megatron-main zsingle megatron-core  ${OPENSSL_LIBRARIES}  z  pthread sqlitecpp sqlite3 dl  )
ENDIF()


install(TARGETS mt-test-${PORT}.exe DESTINATION ${BIN_TARGET_DIR})

