cmake_minimum_required (VERSION 3.0)

add_library(prodtestWebServer STATIC

prodtestWebServerService.cpp
)

target_link_libraries(prodtestWebServer   megatron-core pthread)



