

add_library(core-error-dispatcher SHARED
errorDispatcher.cpp
errorDispatcherService.cpp
errorDispatcherService.h
)
target_link_libraries(core-error-dispatcher megatron-core  pthread )
install(TARGETS core-error-dispatcher DESTINATION ${PLUGIN_TARGET_DIR})

