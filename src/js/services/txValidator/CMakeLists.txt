#
# add_library(node3-service SHARED
#     txValidator.cpp
#     txValidatorService.cpp
# )


#target_link_libraries(node3-service bctools megatron-core mtjscommon sodium pthread gmp bls384_256 mcl rocksdb sodium crypto)
# install(TARGETS node-service DESTINATION ${PLUGIN_TARGET_DIR})
include_directories(/usr/local/include)
add_library(node3-serviceS STATIC
    txValidator.cpp
    txValidatorService.cpp
)



