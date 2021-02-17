cmake_minimum_required (VERSION 3.13)

project(vulkan CXX)

set(INCLUDE
    include/vulkan/vk_mem_alloc.h
    include/vulkan/vk_mem_alloc.hpp
    include/vulkan/vulkan.h
    include/vulkan/vulkan.hpp
    include/vulkan/vulkan_decl.h
)

set(SOURCE
    source/vulkan_decl.cpp
)


source_group("include" FILES ${INCLUDE})
source_group("source"  FILES ${SOURCE})

add_library(vulkan STATIC ${INCLUDE} ${SOURCE})
