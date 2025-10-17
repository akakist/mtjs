




add_library(zsingle STATIC
../dfsCaps/CapsAlgorithm.cpp

../dfsCaps/dfsCapsService.cpp
../dfsReferrer/dfsReferrerService.cpp
../dfsReferrer/uplinkconnectionstate.cpp
../dfsReferrer/uriReferals.cpp
../errorDispatcher/errorDispatcherService.cpp
../http/httpService.cpp
../http/mime_types.cpp
../objectProxy/objectProxyService.cpp
../oscar/oscarService.cpp
../referrerClient/referrerClientService.cpp
../rpc/rpcService.cpp
../socket/socketService.cpp
../ssl/ssl_reg.cpp
../ssl/st_rsa_impl.cpp
../telnet/telnetService.cpp
../timer/timerService.cpp
../webHandler/webHandlerService.cpp

../geoip/CapsGeoIP.cpp
../geoip/geoip_impl.cpp
../geoip/geoip_reg.cpp
../webSocketClient/webSocketClientService.cpp



)

