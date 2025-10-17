
add_library(dfs-caps SHARED

dfsCaps.cpp
dfsCapsService.cpp
CapsAlgorithm.cpp
CapsAlgorithm.h
dfsCapsService.h

)
target_link_libraries(dfs-caps megatron-core   pthread )
install(TARGETS dfs-caps DESTINATION ${PLUGIN_TARGET_DIR})

