

include_directories(../../qjsi ../api)

add_executable(test_runner
test_runner.cpp
)
target_link_libraries(test_runner contract-api quickjs-i bctools   megatron-core megatron-main sodium     crypto  blst yyjson )




