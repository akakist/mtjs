cmake_minimum_required(VERSION 4.2)
project(quickjs)

file(STRINGS VERSION version)

set(quickjs_src 
  cutils_i.c
  dtoa_i.c
  libregexp_i.c
  libunicode_i.c
  quickjs-libc_i.c
  quickjs_i.c
)
add_subdirectory(test)

set(quickjs_def CONFIG_VERSION="${version}" _GNU_SOURCE CONFIG_BIGNUM)

add_library(quickjs-i ${quickjs_src})
target_compile_definitions(quickjs-i PRIVATE ${quickjs_def} )
set_target_properties(quickjs-i PROPERTIES PUBLIC_HEADER "quickjs_i.h;quickjs-libc_i.h")

add_library(quickjs-dumpleaks ${quickjs_src})
target_compile_definitions(quickjs-dumpleaks PRIVATE ${quickjs_def} DUMP_LEAKS=1)

if(UNIX OR MINGW)
  find_package(Threads)
  target_link_libraries(quickjs-i ${CMAKE_DL_LIBS} m Threads::Threads)
  target_link_libraries(quickjs-dumpleaks ${CMAKE_DL_LIBS} m Threads::Threads)
endif()

# install
include(GNUInstallDirs)
install(TARGETS quickjs-i
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/quickjs
        ARCHIVE       DESTINATION ${CMAKE_INSTALL_LIBDIR}/quickjs
        LIBRARY       DESTINATION ${CMAKE_INSTALL_LIBDIR}/quickjs
        )
