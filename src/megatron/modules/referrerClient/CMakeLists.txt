add_definitions(-DWITH_IO)



add_library(ReferrerClient SHARED

referrerClient.cpp
referrerClientService.cpp
referrerClientService.h
uplinkconnectionstate_client.h

)
target_link_libraries(ReferrerClient megatron-core  pthread )
install(TARGETS ReferrerClient DESTINATION ${PLUGIN_TARGET_DIR})

