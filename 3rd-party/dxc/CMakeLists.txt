cmake_minimum_required (VERSION 3.19)

project(dxc CXX)

set(INCLUDE
    include/dxc/dxcapi.h
    include/dxc/dxcapi.use.h
)

source_group("include" FILES ${INCLUDE})
add_library(dxc INTERFACE ${INCLUDE})
