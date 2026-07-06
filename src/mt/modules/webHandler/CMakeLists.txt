
add_library(core-webHandler SHARED
    webHandler.cpp
    webHandlerService.cpp
    webHandlerService.h
)

target_link_libraries(core-webHandler megatron-core  pthread )

install(TARGETS core-webHandler DESTINATION ${PLUGIN_TARGET_DIR})

