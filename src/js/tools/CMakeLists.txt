add_executable(jsi_pack pack.cpp)
add_executable(jsi_unpack unpack.cpp)
add_executable(apply_edit apply_edit.cpp)


add_executable(keygen_users keygen_users.cpp)

target_link_libraries(keygen_users crypto sodium)

add_executable(keygen_genesis keygen_genesis.cpp)
target_link_libraries(keygen_genesis crypto sodium blst)
