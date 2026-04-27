

add_executable(mtjs
mainMTJS.cpp
main_.cpp

)
set_target_properties(mtjs PROPERTIES LINK_SEARCH_START_STATIC ON)
set_target_properties(mtjs PROPERTIES LINK_SEARCH_END_STATIC ON)


link_libraries("-static")
link_directories(/usr/local/lib)



set(OPENSSL_USE_STATIC_LIBS ON)

if(NOT APPLE)
set(linuxlib     mariadbclient )
endif()

if(APPLE)
target_link_libraries(mtjs  node2-serviceS node-serviceS node3-serviceS node4-serviceS  node5-serviceS  node6-serviceS node7-serviceS
     mtjsmoduleS mtjscommon    mtjsnet mtjsutils bctools  quickjs megatron-main   zsingle megatron-core   ${linuxlib}  yyjson ssh zstd lz4 bz2 snappy blst

#curl  
${OPENSSL_LIBRARIES}
  z pthread dl

  )

else()
target_link_libraries(mtjs -static  -Wl,--start-group node2-serviceS node-serviceS node3-serviceS node4-serviceS node5-serviceS node6-serviceS node7-serviceS
     mtjsmoduleS mtjscommon     mtjsnet  mtjsutils bctools quickjs megatron-main   zsingle megatron-core sodium   rocksdb SQLiteCpp sqlite3 crypto  blst
      ${linuxlib} yyjson zstd lz4 bz2 snappy

#curl  psl idn2 unistring nghttp2
ssl crypto
  z pthread dl
  -Wl,--end-group
  )

endif()


link_directories(${OPENSSL_INCLUDE_DIR}/../lib)

install(TARGETS mtjs DESTINATION /usr/local/bin)
