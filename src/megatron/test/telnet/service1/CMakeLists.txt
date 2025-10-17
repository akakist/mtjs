cmake_minimum_required (VERSION 3.0)

add_library(teldemo1 STATIC

teldemo1Service.cpp
)

target_link_libraries(teldemo1   megatron-core pthread)



