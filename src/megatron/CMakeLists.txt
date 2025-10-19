cmake_minimum_required (VERSION 3.16)

project (megatron)

if(UNIX AND NOT APPLE)
SET(LINUX true)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fPIC ")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC ")

if(LINUX)
    set(CMAKE_SHARED_LINKER_FLAGS "-Xlinker --export-dynamic -Xlinker -zdefs ")
else()
    set(CMAKE_PREFIX_PATH $ENV{HOME}/Qt/5.15.2/clang_64)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)

include(CTest)

find_package(OpenSSL REQUIRED)


if (OPENSSL_FOUND)
message(STATUS "OPENSSL_LIBRARIES ${OPENSSL_LIBRARIES}" )
message(STATUS "OPENSSL_INCLUDE_DIR ${OPENSSL_INCLUDE_DIR}" )
message(STATUS "OPENSSL_LIBRARY_DIR ${OPENSSL_LIBRARY_DIR}" )
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()


find_package(PkgConfig REQUIRED)
pkg_search_module(MYSQL REQUIRED mariadb)


if (MYSQL_FOUND)
    message(STATUS "MYSQL_INCLUDE_DIRS: ${MYSQL_INCLUDE_DIRS}")
    message(STATUS "MYSQL_LIBRARY_DIRS: ${MYSQL_LIBRARY_DIRS}")
    message(STATUS "MYSQL_LIBRARIES: ${MYSQL_LIBRARIES}")
    message(STATUS "MYSQL_VERSION: ${MYSQL_VERSION}")


    include_directories(${MYSQL_INCLUDE_DIRS})
    link_directories(${MYSQL_LIBRARY_DIRS})
endif()

pkg_search_module(nlohmann_json REQUIRED nlohmann_json)

if (nlohmann_json_FOUND)
    include_directories(${nlohmann_json_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "nlohmann/json not found!")
endif()

find_package(Git)

if(PROJECT_IS_TOP_LEVEL)
CONFIGURE_FILE( "${PROJECT_SOURCE_DIR}/genum.hpp.in"
                "${CMAKE_BINARY_DIR}/genum.hpp"
                NEWLINE_STYLE UNIX )


CONFIGURE_FILE( "${PROJECT_SOURCE_DIR}/version.h.in"
                "${CMAKE_BINARY_DIR}/version.h"
                NEWLINE_STYLE UNIX )


endif()

if(APPLE)
include_directories(
     )
link_directories(
 )
endif()


include_directories(.)
include_directories( 
    local   . corelib lib main          /usr/local/include
${CMAKE_BINARY_DIR}
)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fPIC -O3")

SET(CAPS_IP "caps.host")
SET(PORT 10100)


IF(LINUX)
SET(DL "dl")
ENDIF()

if(PROJECT_IS_TOP_LEVEL)
SET(PROJECT_BASE_DIR /opt/megatron)
endif()

SET(PLUGIN_TARGET_DIR ${PROJECT_BASE_DIR}/plugin)
SET(BIN_TARGET_DIR ${PROJECT_BASE_DIR}/bin)
SET(LIB_TARGET_DIR ${PROJECT_BASE_DIR}/lib)
SET(LOG_TARGET_DIR ${PROJECT_BASE_DIR}/log)
SET(DATA_TARGET_DIR ${PROJECT_BASE_DIR}/data)
SET(CONFIG_TARGET_DIR ${PROJECT_BASE_DIR}/conf)
SET(CACHE_TARGET_DIR ${PROJECT_BASE_DIR}/cache)



CONFIGURE_FILE( "${PROJECT_SOURCE_DIR}/megatron_config.h.in"
                "${CMAKE_BINARY_DIR}/megatron_config.h"
                NEWLINE_STYLE UNIX )



ADD_SUBDIRECTORY(corelib)
ADD_SUBDIRECTORY(main)
ADD_SUBDIRECTORY(lib)
ADD_SUBDIRECTORY(apps)
ADD_SUBDIRECTORY(test)
ADD_SUBDIRECTORY(modules)




