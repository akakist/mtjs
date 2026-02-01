

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
target_link_libraries(mtjs  
     mtjsmoduleS mtjscommon    jsnet  jsutils  quickjs megatron-main   zsingle megatron-core   ${linuxlib}  ssh quickjs

#curl  
${OPENSSL_LIBRARIES}
  z pthread dl

  )

else()
target_link_libraries(mtjs  -Wl,--start-group
     mtjsmoduleS mtjscommon    zmysql jsnet  jsutils  quickjs megatron-main   zsingle megatron-core 
      ${linuxlib}

#curl  psl idn2 unistring nghttp2
ssl crypto
  z pthread dl
  -Wl,--end-group
  )

endif()


link_directories(${OPENSSL_INCLUDE_DIR}/../lib)

install(TARGETS mtjs DESTINATION /usr/local/bin)
