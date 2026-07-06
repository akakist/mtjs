
add_library(core-oscar SHARED
    oscar.cpp
    oscarService.cpp
    bufferVerify.h
oscarService.h

)


target_link_libraries(core-oscar megatron-core  pthread )

install(TARGETS core-oscar DESTINATION ${PLUGIN_TARGET_DIR})

