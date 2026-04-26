

cmake_minimum_required (VERSION 3.16)


include_directories(common quickjs . bc_tools Event)

#ADD_SUBDIRECTORY(megatron)

ADD_SUBDIRECTORY(services)
ADD_SUBDIRECTORY(bc_tools)
ADD_SUBDIRECTORY(test)
ADD_SUBDIRECTORY(jslib)
ADD_SUBDIRECTORY(mono)
ADD_SUBDIRECTORY(common)
ADD_SUBDIRECTORY(grain)



