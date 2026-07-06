




add_library(zsingle STATIC

../errorDispatcher/errorDispatcherService.cpp
../http/httpService.cpp
../http/mime_types.cpp
../objectProxy/objectProxyService.cpp
../oscar/oscarService.cpp
../rpc/rpcService.cpp
../socket/socketService.cpp
../ssl/ssl_reg.cpp
../ssl/st_rsa_impl.cpp
../telnet/telnetService.cpp
../timer/timerService.cpp
../webHandler/webHandlerService.cpp

../webSocketClient/webSocketClientService.cpp



)

