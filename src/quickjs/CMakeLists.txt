


include_directories( 
common
${pcre2_BINARY_DIR}/
${cppcodec_SOURCE_DIR}


)
link_directories( ${OPENSSL_INCLUDE_DIR})

if(APPLE)
    add_definitions(-DCONFIG_VERSION=\"2024-02-14\" -DCONFIG_BIGNUM=1 -D_GNU_SOURCE -D_DEFAULT_SOURCE)
else()
    add_definitions(-DCONFIG_VERSION=\"2024-02-14\" -DCONFIG_BIGNUM=1 -DHAVE_CLOSEFROM -D_GNU_SOURCE -D_DEFAULT_SOURCE)
endif()


add_library( qjsl STATIC
cutils.c
cutils.h
libbf.c
libbf.h
libregexp.c
libregexp.h
libregexp-opcode.h
libunicode.c
libunicode.h
libunicode-table.h
list.h

quickjs-atom.h
quickjs.c
quickjs.h
quickjs-libc.c
quickjs-libc.h
quickjs-opcode.h
unicode_gen.c
unicode_gen_def.h

repl.c
qjscalc.c

../jslib/fsjs/asyncFSio.cpp
)

if(NOT APPLE)
set(linuxlib psl idn2 unistring zstd)
endif()

