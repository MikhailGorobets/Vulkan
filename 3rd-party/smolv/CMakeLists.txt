cmake_minimum_required (VERSION 3.13)

project(smolv CXX)

set(INCLUDE
   include/smolv/smolv.h
)

set(SOURCE
   source/smolv.cpp
)

source_group("include" FILES ${INCLUDE})
source_group("source"  FILES ${SOURCE})

add_library(smolv STATIC ${INCLUDE} ${SOURCE})