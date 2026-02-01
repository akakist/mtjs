
link_directories(/usr/lib/x86_64-linux-gnu)

add_library(mtjsmodule SHARED
    mtjs.cpp
    mtjsService.cpp

    mtjs_http.cpp
    mtjs_ws_server.cpp
    mtjs_ws_connection.cpp
    mtjs_async_executed.cpp
    mtjs_startService.cpp
    mtjs_rpc.cpp


)
add_library(mtjsmoduleS STATIC
    mtjsService.cpp
    mtjsService.h
    mtjs_http.cpp
    mtjs_ws_server.cpp
    mtjs_ws_connection.cpp
    mtjs_async_executed.cpp
    mtjs_startService.cpp
    mtjs_rpc.cpp
    mtjsService.h
)
if(NOT APPLE)

set(linuxlib
  ${MYSQL_LIBRARIES} ${PostgreSQL_LIBRARIES} ${OPENSSL_LIBRARIES}  ${SQLite3_LIBRARIES} sqlite3   z  
)
endif()
if(APPLE)
target_link_libraries(mtjsmodule   jsnet jsutils mtjscommon  quickjs megatron-core    pthread curl   ${linuxlib}   )
else()
target_link_libraries( mtjsmodule
#"LINKER:--start-group"
"-Wl,--start-group"
			   jsnet jsutils mtjscommon  quickjs megatron-core    
			"-Wl,--end-group" 
#			"LINKER:--end-group"
			pthread   ${linuxlib} 
			)
endif()

install(TARGETS mtjsmodule DESTINATION ${PLUGIN_TARGET_DIR})

