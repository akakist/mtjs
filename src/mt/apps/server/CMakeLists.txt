

include_directories(../../main ${CMAKE_BINARY_DIR})

add_executable(megatrond 
mainApp.cpp


)


    IF(FreeBSD)
        target_link_libraries(megatrond megatron-main  megatron-core  z  pthread )
    ELSE()
	target_link_libraries(megatrond megatron-main  megatron-core  z  pthread dl )
    ENDIF()

install(TARGETS megatrond DESTINATION ${BIN_TARGET_DIR})

