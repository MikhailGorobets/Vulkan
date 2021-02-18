cmake_minimum_required (VERSION 3.19)

project(hlslpp CXX)

set(INCLUDE
    include/hlslpp/hlsl++.h
)

source_group("include" FILES ${INCLUDE})
add_library(hlslpp INTERFACE ${INCLUDE})
