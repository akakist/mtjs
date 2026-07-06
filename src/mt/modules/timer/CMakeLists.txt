
add_library(core-timer SHARED
    timer.cpp
    timerService.cpp
timerService.h
)

target_link_libraries(core-timer megatron-core   pthread )

install(TARGETS core-timer DESTINATION ${PLUGIN_TARGET_DIR})

