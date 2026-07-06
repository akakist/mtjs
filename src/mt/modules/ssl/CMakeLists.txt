
add_library(core-ssl SHARED
    ssl.cpp
    ssl_reg.cpp
    st_rsa_impl.cpp
st_rsa_impl.h
)

target_link_libraries(core-ssl megatron-core  ${OPENSSL_LIBRARIES}  pthread  dl )

install(TARGETS core-ssl DESTINATION ${PLUGIN_TARGET_DIR})

