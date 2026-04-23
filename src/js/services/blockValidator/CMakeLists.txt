
 add_library(node2-service SHARED
     blockValidator.cpp
     blockValidatorService.cpp
 )


target_link_libraries(node2-service bctools megatron-core mtjscommon sodium pthread gmp bls384_256 mcl rocksdb sodium crypto)
# install(TARGETS node-service DESTINATION ${PLUGIN_TARGET_DIR})
include_directories(/usr/local/include)
add_library(node2-serviceS STATIC
    blockValidator.cpp
    blockValidatorService.cpp
)



