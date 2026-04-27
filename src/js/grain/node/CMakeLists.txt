
# add_library(node-service SHARED
#     node.cpp
#     nodeService.cpp
#     root_contract.cpp
#     tr_exec.cpp
#     hasher.cpp
# )


# target_link_libraries(node-service bctools megatron-core mtjscommon sodium pthread gmp bls384_256 mcl rocksdb sodium crypto)
# install(TARGETS node-service DESTINATION ${PLUGIN_TARGET_DIR})
include_directories(/usr/local/include)
add_library(node-serviceS STATIC
    node.cpp
    nodeService.cpp
    sync.cpp
    heart_beat.cpp
)



