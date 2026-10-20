cmake_minimum_required(VERSION 4.2)
project(quickjs)

file(STRINGS VERSION version)

set(quickjsi_src 
  cutils_i.c
  libunicode_i.c
  quickjs-libc_i.c
  quickjs_i.c
)
add_subdirectory(test)

set(quickjsi_def CONFIG_VERSION="${version}" _GNU_SOURCE CONFIG_BIGNUM)

add_library(quickjs-i ${quickjsi_src})
target_compile_definitions(quickjs-i PRIVATE ${quickjsi_def} )
set_target_properties(quickjs-i PROPERTIES PUBLIC_HEADER "quickjsi_i.h;quickjs-libc_i.h")

add_library(quickjs-dumpleaks2 ${quickjsi_src})
target_compile_definitions(quickjs-dumpleaks2 PRIVATE ${quickjsi_def} DUMP_LEAKS=1)

if(UNIX OR MINGW)
  find_package(Threads)
  target_link_libraries(quickjs-i ${CMAKE_DL_LIBS} m Threads::Threads)
  target_link_libraries(quickjs-dumpleaks2 ${CMAKE_DL_LIBS} m Threads::Threads)
endif()

# install
include(GNUInstallDirs)
install(TARGETS quickjs-i
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/quickjs
        ARCHIVE       DESTINATION ${CMAKE_INSTALL_LIBDIR}/quickjs
        LIBRARY       DESTINATION ${CMAKE_INSTALL_LIBDIR}/quickjs
        )
