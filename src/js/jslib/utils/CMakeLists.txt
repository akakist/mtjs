


add_library(jsutils STATIC
#./yyjson/yyjson.c
./yyjson/parse_yyjson.cpp
#./yyjson/yyjson.h
./timer/timer_root.cpp
timer/timeout.cpp
eventEmitter.cpp
utils.cpp
telnet_js.cpp
telnet_request.cpp
asyncFSio.cpp
           )

