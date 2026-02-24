

cmake_minimum_required (VERSION 3.16)


include_directories(common quickjs .)

#ADD_SUBDIRECTORY(megatron)

ADD_SUBDIRECTORY(services)
#ADD_SUBDIRECTORY(quickjs)
#ADD_SUBDIRECTORY(test)
ADD_SUBDIRECTORY(jslib)
ADD_SUBDIRECTORY(mono)
ADD_SUBDIRECTORY(common)



