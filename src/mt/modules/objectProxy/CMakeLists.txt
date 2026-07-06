
add_library(core-objectProxy SHARED
    objectProxy.cpp
    objectProxyService.cpp
objectProxyService.h
)

target_link_libraries(core-objectProxy megatron-core  pthread )

install(TARGETS core-objectProxy DESTINATION ${PLUGIN_TARGET_DIR})

