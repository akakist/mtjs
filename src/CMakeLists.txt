

cmake_minimum_required (VERSION 3.16)

project (MTJS)

if(UNIX AND NOT APPLE)
SET(LINUX true)
endif()


set(APP_NAME "mtjs server")

set(M1 true)



SET(DFS true)
if(LINUX)
    set(CMAKE_SHARED_LINKER_FLAGS "-Xlinker --export-dynamic -Xlinker -zdefs")
else()

endif()

    include(CTest)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)


include(FetchContent)

FetchContent_Declare(
    yyjson
    URL https://github.com/ibireme/yyjson/archive/refs/tags/0.8.0.tar.gz
)
FetchContent_MakeAvailable(yyjson)

include_directories(${yyjson_SOURCE_DIR}/src)
message (STATUS "yyjson dir " ${yyjson_INCLUDE_DIRS})
message (STATUS "yyjson d2ir " ${yyjson_SOURCE_DIR})

find_package(PkgConfig REQUIRED)

find_package(SQLite3 REQUIRED)
find_package(PostgreSQL REQUIRED)



pkg_search_module(nlohmann_json REQUIRED nlohmann_json)

if (nlohmann_json_FOUND)
    include_directories(${nlohmann_json_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "nlohmann/json not found!")
endif()

if(NOT APPLE)
pkg_search_module(mysql REQUIRED mariadb)

if (MYSQL_FOUND)
    message(STATUS "MYSQL_INCLUDE_DIRS: ${MYSQL_INCLUDE_DIRS}")
    message(STATUS "MYSQL_LIBRARY_DIRS: ${MYSQL_LIBRARY_DIRS}")
    message(STATUS "MYSQL_LIBRARIES: ${MYSQL_LIBRARIES}")
    message(STATUS "MYSQL_VERSION: ${MYSQL_VERSION}")


    include_directories(${MYSQL_INCLUDE_DIRS})
    link_directories(${MYSQL_LIBRARY_DIRS})
endif()
endif()

find_package(OpenSSL REQUIRED)


if (OPENSSL_FOUND)
message(STATUS "OPENSSL_LIBRARIES ${OPENSSL_LIBRARIES}" )
message(STATUS "OPENSSL_INCLUDE_DIR ${OPENSSL_INCLUDE_DIR}" )
message(STATUS "OPENSSL_LIBRARY_DIR ${OPENSSL_LIBRARY_DIR}" )
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

if(NOT APPLE)
find_package(SQLite3 REQUIRED)
if (SQLite3_FOUND)
    include_directories(${SQLite3_INCLUDE_DIRS})
    message(STATUS "Found SQLite3: ${SQLite3_LIBRARIES}")
else()
    message(FATAL_ERROR "SQLite3 not found")
endif()

find_package(PostgreSQL REQUIRED)

include_directories(${PostgreSQL_INCLUDE_DIRS})
link_directories(${PostgreSQL_LIBRARY_DIRS})

message(STATUS "PostgreSQL_INCLUDE_DIRS ${PostgreSQL_INCLUDE_DIRS}" )
message(STATUS "PostgreSQL_LIBRARY_DIRS ${PostgreSQL_LIBRARY_DIRS}" )
endif()

if(NOT APPLE)


endif()



include_directories(.)

include_directories(megatron megatron/main megatron/corelib)

OPTION(MT_CORE_SERVICES_SINGLE_PLUGING "Build apps/core-plugins - all core plugins in one file" ON)

include_directories( megatron/local  megatron . megatron/corelib  megatron/lib/SQLiteCpp  megatron/lib  megatron/main

/usr/local/include
lib/cpp-jwt/include


    )




if(APPLE)
    set(ENV{PKG_CONFIG_PATH} "/usr/local/mysql/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    pkg_check_modules(OPENSSL REQUIRED openssl)
    
    pkg_check_modules(MARIADB REQUIRED libmariadb)
    if(MARIADB_FOUND)
        message(STATUS "Found MariaDB via pkg-config!")
        include_directories(${MARIADB_INCLUDE_DIRS})
        link_directories(${MARIADB_LIBRARY_DIRS})
    else()
        message(WARNING "MariaDB pkg-config file not found. Will try manual fallback paths.")
    endif()
    
include_directories(
)
link_directories(
)
endif()

include_directories(	src/mtjs
			megatron/corelib 
			megatron/main 
			megatron/local  
			megatron/lib/jpeg-9d

			megatron  .  svs megatron/lib megatron/lib/sqlite3

			)

include_directories(system /usr/local/include quickjs)

include_directories(${CMAKE_BINARY_DIR} common)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fPIC -O3 -march=native  -DNDEBUG ")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -O3 -march=native  -DNDEBUG ")
#№-flto -fomit-frame-pointer
# set(CMAKE_BUILD_TYPE Release)
# set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -flto")
# # Или для конкретных таргетов
# //target_compile_options(your_target PRIVATE -O3 -march=native)


link_directories( /usr/local/lib )
IF(APPLE)
    SET(CMAKE_MACOSX_RPATH 1)
ENDIF()
SET(DISABLE_UPNP on)


set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (APPLE) 
      set(CMAKE_MACOSX_RPATH ON) 
      SET(CMAKE_SKIP_BUILD_RPATH TRUE) 
      SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE) 
      SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE) 
endif () 

CONFIGURE_FILE( "${PROJECT_SOURCE_DIR}/genum.hpp.in"
                "${CMAKE_BINARY_DIR}/genum.hpp"
                NEWLINE_STYLE UNIX )

FILE(READ version.txt VERSION)

CONFIGURE_FILE( "${PROJECT_SOURCE_DIR}/version.h.in"
                "${CMAKE_BINARY_DIR}/version.h"
                NEWLINE_STYLE UNIX )


SET(CAPS_IP "x.testquant.com")
SET(PORT 10100)
add_definitions()
#-DDEBUG  -DMUTEX_INSPECTOR_DEBUG

    add_definitions(  -DCURL_POLL_ADD=5)


IF(LINUX)
SET(DL "dl")
ENDIF()

SET(PROJECT_BASE_DIR /opt/mtjs)
SET(PLUGIN_TARGET_DIR ${PROJECT_BASE_DIR}/plugin)
SET(BIN_TARGET_DIR ${PROJECT_BASE_DIR}/bin)
SET(LIB_TARGET_DIR ${PROJECT_BASE_DIR}/lib)
SET(LOG_TARGET_DIR ${PROJECT_BASE_DIR}/log)
SET(DATA_TARGET_DIR ${PROJECT_BASE_DIR}/data)
SET(CONFIG_TARGET_DIR ${PROJECT_BASE_DIR}/conf)
SET(CACHE_TARGET_DIR ${PROJECT_BASE_DIR}/cache)




ADD_SUBDIRECTORY(megatron)

ADD_SUBDIRECTORY(services)
ADD_SUBDIRECTORY(quickjs)
#ADD_SUBDIRECTORY(test)
ADD_SUBDIRECTORY(jslib)
ADD_SUBDIRECTORY(mono)
ADD_SUBDIRECTORY(common)



