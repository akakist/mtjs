
add_library(core-modules SHARED
core-modules.cpp 



../../modules/dfsCaps/CapsAlgorithm.cpp
../../modules/dfsCaps/dfsCapsService.cpp
../../modules/dfsReferrer/dfsReferrerService.cpp
../../modules/dfsReferrer/uplinkconnectionstate.cpp
../../modules/dfsReferrer/uriReferals.cpp
../../modules/errorDispatcher/errorDispatcherService.cpp
../../modules/geoip/CapsGeoIP.cpp
../../modules/geoip/geoip_impl.cpp
../../modules/geoip/geoip_reg.cpp
../../modules/http/httpService.cpp
../../modules/http/mime_types.cpp
../../modules/objectProxy/objectProxyService.cpp
../../modules/oscar/oscarService.cpp
../../modules/oscarSecure/oscarSecureService.cpp
../../modules/oscarSecure/oscarSecureUser.cpp
../../modules/referrerClient/referrerClientService.cpp
../../modules/rpc/rpcService.cpp
../../modules/rpc/upnp_tr.c
../../modules/socket/socketService.cpp
../../modules/ssl/ssl_reg.cpp
../../modules/ssl/st_rsa_impl.cpp
../../modules/telnet/telnetService.cpp
../../modules/timer/timerService.cpp
../../modules/webHandler/webHandlerService.cpp

)
target_link_libraries(core-modules megatron-core ${OPENSSL_LIBRARIES})
install(TARGETS core-modules DESTINATION ${PLUGIN_TARGET_DIR})

