

cmake_minimum_required (VERSION 3.2)

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

#FetchContent_Declare(
#    yyjson
#    URL https://github.com/ibireme/yyjson/archive/refs/tags/0.12.0.tar.gz
#)
#FetchContent_MakeAvailable(yyjson)

#FetchContent_Declare(
#    xyjson
#    URL https://github.com/lymslive/xyjson/archive/refs/tags/v1.0.0.tar.gz
#)
#FetchContent_MakeAvailable(xyjson)


#message (STATUS "YYJSON: "  ${yyjson_SOURCE_DIR}/src)
#message (STATUS "XYJSON: "  ${xyjson_SOURCE_DIR})
#include_directories( ${yyjson_SOURCE_DIR}/src )
#include_directories( ${xyjson_SOURCE_DIR}/include)

find_package(PkgConfig REQUIRED)
find_package(yyjson REQUIRED)
#find_package(xyjson REQUIRED)

find_package(SQLite3 REQUIRED)
find_package(PostgreSQL REQUIRED)

# Locate libsodium through pkg-config. This is required for Homebrew prefixes
# on macOS and works unchanged with system packages on Linux.
pkg_check_modules(SODIUM REQUIRED libsodium)
include_directories(${SODIUM_INCLUDE_DIRS})
link_directories(${SODIUM_LIBRARY_DIRS})

pkg_check_modules(ROCKSDB_PKG REQUIRED rocksdb)
include_directories(${ROCKSDB_PKG_INCLUDE_DIRS})
link_directories(${ROCKSDB_PKG_LIBRARY_DIRS})



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
find_package(RocksDB REQUIRED)

find_package(OpenSSL REQUIRED)


if (OPENSSL_FOUND)
message(STATUS "OPENSSL_LIBRARIES ${OPENSSL_LIBRARIES}" )
message(STATUS "OPENSSL_INCLUDE_DIR ${OPENSSL_INCLUDE_DIR}" )
message(STATUS "OPENSSL_LIBRARY_DIR ${OPENSSL_LIBRARY_DIR}" )
    include_directories(${OPENSSL_INCLUDE_DIR})
    # Some legacy targets still link OpenSSL by bare "crypto" and "ssl" names.
    # Add the detected library directory so those links work on macOS too.
    get_filename_component(OPENSSL_LIBRARY_DIR "${OPENSSL_CRYPTO_LIBRARY}" DIRECTORY)
    link_directories("${OPENSSL_LIBRARY_DIR}")
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



include_directories(. qjsi)

# Keep only one copy of core headers in the search path. The megatron
# directory duplicates mt and causes redefinitions on macOS/clang.
include_directories(mt/main mt/corelib)

OPTION(MT_CORE_SERVICES_SINGLE_PLUGING "Build apps/core-plugins - all core plugins in one file" ON)

include_directories( mt/local  mt . mt/corelib  mt/lib/SQLiteCpp  mt/lib  mt/main

/usr/local/include
lib/cpp-jwt/include


    )




if(APPLE)
    pkg_check_modules(OPENSSL REQUIRED openssl)

    # The MariaDB server package provides pkg-config metadata with different
    # headers. Use Connector/C because this project includes <mariadb/mysql.h>.
    find_path(MARIADB_INCLUDE_DIR mariadb/mysql.h
        PATHS /opt/homebrew/opt/mariadb-connector-c/include
              /usr/local/opt/mariadb-connector-c/include
    )
    unset(MARIADB_LIBRARY CACHE)
    find_library(MARIADB_LIBRARY NAMES mariadb
        PATHS /opt/homebrew/opt/mariadb-connector-c/lib
              /usr/local/opt/mariadb-connector-c/lib
        NO_DEFAULT_PATH
    )
    if(NOT MARIADB_INCLUDE_DIR OR NOT MARIADB_LIBRARY)
        message(FATAL_ERROR "MariaDB Connector/C with mariadb/mysql.h was not found")
    endif()

    get_filename_component(MARIADB_LIBRARY_DIR "${MARIADB_LIBRARY}" DIRECTORY)
    include_directories("${MARIADB_INCLUDE_DIR}")
    link_directories("${MARIADB_LIBRARY_DIR}")
    set(MARIADB_LIBRARIES "${MARIADB_LIBRARY}")

include_directories(
)
link_directories( qjsi quickjs)
endif()

include_directories(	src/mtjs
			mt/corelib 
			mt/main 
			mt/local  
			mt/lib/jpeg-9d

			mt  .  svs mt/lib mt/lib/sqlite3

			)

include_directories(system /usr/local/include quickjs)

include_directories(${CMAKE_BINARY_DIR} common)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fPIC -O3 -march=native  -DNDEBUG -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -O3 -march=native  -DNDEBUG -g")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fPIC -O0 -g -fsanitize=address -fno-omit-frame-pointer")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -O0 -g -fsanitize=address -fno-omit-frame-pointer")
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
#set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")

#ser
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

#CONFIGURE_FILE( "${PROJECT_SOURCE_DIR}/genum.hpp.in"
#                "${CMAKE_BINARY_DIR}/genum.hpp"
#                NEWLINE_STYLE UNIX )

FILE(READ version.txt VERSION)

CONFIGURE_FILE( "${PROJECT_SOURCE_DIR}/version.h.in"
                "${CMAKE_BINARY_DIR}/version.h"
                NEWLINE_STYLE UNIX )


SET(CAPS_IP "x.testquant.com")
SET(PORT 10100)
add_definitions()
#-DMEMLEAK_CHECK 
# -DDEBUG  -DMUTEX_INSPECTOR_DEBUG
#-DDEBUG  -DMUTEX_INSPECTOR_DEBUG -DWEBDUMP -DDEBUG  -DMUTEX_INSPECTOR_DEBUG

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




ADD_SUBDIRECTORY(mt)

#ADD_SUBDIRECTORY(xpp)
ADD_SUBDIRECTORY(js)
ADD_SUBDIRECTORY(quickjs)
ADD_SUBDIRECTORY(qjsi)
#ADD_SUBDIRECTORY(bc)



