cmake_minimum_required (VERSION 3.19)

project(Vulkan CXX)

set(PROJECT_CONTENT            "${CMAKE_CURRENT_SOURCE_DIR}/content")
set(PROJECT_DIRECTORY          "${CMAKE_CURRENT_SOURCE_DIR}")
list(PREPEND CMAKE_MODULE_PATH "${PROJECT_DIRECTORY}/cmake")

include(cmake_settings)
include(compiler_settings)

include(3rd-party/dxc)
include(3rd-party/fmt)
include(3rd-party/glfw)
include(3rd-party/vulkan)
include(3rd-party/imgui)
include(3rd-party/implot)
include(3rd-party/spirv-cross)



include_directories(PUBLIC "source/HAL/interface")

add_subdirectory(source/HAL)

set(SOURCE "source/Main.cpp")

source_group("include" FILES ${INCLUDE})
source_group("source" FILES  ${SOURCE})

add_executable(Vulkan ${INCLUDE} ${SOURCE})
set_target_properties(Vulkan PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_DIRECTORY}")
target_link_libraries(Vulkan PRIVATE fmt glfw spirv-cross-core spirv-cross-hlsl vulkan imgui implot HAL)
