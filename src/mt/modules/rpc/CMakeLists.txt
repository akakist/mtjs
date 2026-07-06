add_library(core-rpc SHARED
            rpc.cpp
            rpcService.cpp
	    rpcService.h
           )

target_link_libraries(core-rpc megatron-core  pthread )

install(TARGETS core-rpc DESTINATION ${PLUGIN_TARGET_DIR})


