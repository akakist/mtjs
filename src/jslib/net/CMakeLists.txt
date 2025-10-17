

add_library(jsnet STATIC

./http/http_server_root.cpp
./http/http_server_request.cpp
./http/http_server_response.h
./http/ws_server_connection.h
./http/http_server.h
./http/http_server_request.h
./http/http_server_response.cpp
./http/http_server.cpp
./http/ws_server_connection.cpp
# ./http/fetch.cpp
pipeReader.cpp
io/iostream.cpp
js_mysql.cpp
rpc.cpp
rpc_request.cpp
rpc_response.cpp
asyncExecute.cpp
           )


           
