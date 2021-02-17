cmake_minimum_required (VERSION 3.13)

project(spirv CXX)

set(INCLUDE
    include/spirv/spirv_cross.hpp
    include/spirv/spirv_parser.hpp
)

source_group("include" FILES ${INCLUDE})
add_library(spirv  INTERFACE ${INCLUDE})