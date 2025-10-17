

add_library(core-logger SHARED
logger.cpp
loggerService.cpp
loggerService.h
)
target_link_libraries(core-logger megatron-core  pthread )
install(TARGETS core-logger DESTINATION ${PLUGIN_TARGET_DIR})

