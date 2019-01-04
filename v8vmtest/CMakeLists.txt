cmake_minimum_required (VERSION 2.8)

project (v8vmtest)

INCLUDE_DIRECTORIES(../deps/v8/include)
INCLUDE_DIRECTORIES(../v8vm)

AUX_SOURCE_DIRECTORY(. DIR_SRCS)

ADD_EXECUTABLE(v8vmtest ${DIR_SRCS})

SET(EXTRA_LIBS libv8vm.so 
	libv8.so
	libpthread.so)

TARGET_LINK_LIBRARIES(v8vmtest ${EXTRA_LIBS})
