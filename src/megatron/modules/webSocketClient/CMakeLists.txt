

add_library(core-ws-client SHARED
webSocketClient.cpp
webSocketClientService.cpp
webSocketClientService.h
)
target_link_libraries(core-ws-client megatron-core  pthread )
install(TARGETS core-ws-client DESTINATION ${PLUGIN_TARGET_DIR})

