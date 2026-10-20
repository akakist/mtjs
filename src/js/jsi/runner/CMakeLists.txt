

include_directories(../../qjsi ../api)

add_executable(test_runner

test_runner.cpp
)
target_link_libraries(test_runner  quickjs-i )




