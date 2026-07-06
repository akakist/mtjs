

link_directories(/usr/local/mysql/lib)

add_library(core-mysql SHARED
    mysql.cpp
mysqlService.h    mysqlService.cpp
)
add_library(zmysql STATIC
    mysqlService.cpp
)

target_link_libraries(core-mysql megatron-core  ${MYSQL_LIBRARIES}  ${OPENSSL_LIBRARIES} z  pthread )

install(TARGETS core-mysql DESTINATION ${PLUGIN_TARGET_DIR})

