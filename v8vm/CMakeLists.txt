cmake_minimum_required (VERSION 2.8)

project (libv8vm)

ADD_DEFINITIONS(-DLIBV8VM)
ADD_DEFINITIONS(-DBUILDING_V8_SHARED)

INCLUDE_DIRECTORIES(../deps/v8/include)

AUX_SOURCE_DIRECTORY(. DIR_SRCS)

ADD_SUBDIRECTORY(module)
ADD_SUBDIRECTORY(vm)

ADD_LIBRARY(v8vm SHARED ${DIR_SRCS})

TARGET_LINK_LIBRARIES(v8vm module vm libv8.so libpthread.so)
