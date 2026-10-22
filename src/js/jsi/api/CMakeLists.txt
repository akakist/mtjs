#include_directories(../../qjsi ../api)

add_library(contract-api STATIC
convert.cpp
contract_call.cpp
contract_api.cpp
contract_host.cpp
)



