cmake_minimum_required(VERSION 4.2)
#project(test LANGUAGES CPP)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fPIC -O3 -march=native  -DNDEBUG -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -O3 -march=native  -DNDEBUG -g")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ")

include_directories(..)
add_executable(test_runner
    test_runner.cpp
    )
target_link_libraries(test_runner quickjs-i)
