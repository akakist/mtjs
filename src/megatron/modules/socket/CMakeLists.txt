
add_library(core-socket SHARED
            socket.cpp
            socketService.cpp
            socketService.h

           )

target_link_libraries(core-socket megatron-core  pthread )

install(TARGETS core-socket DESTINATION ${PLUGIN_TARGET_DIR})

