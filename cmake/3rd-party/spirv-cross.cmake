
option(SPIRV_CROSS_CLI "Build the CLI binary. Requires SPIRV_CROSS_STATIC." OFF)
option(SPIRV_CROSS_ENABLE_MSL "Enable MSL target support." OFF)
option(SPIRV_CROSS_ENABLE_REFLECT "Enable JSON reflection target support." OFF)
option(SPIRV_CROSS_ENABLE_C_API "Enable C API wrapper support in static library." OFF)
option(SPIRV_CROSS_ENABLE_UTIL "Enable util module support." OFF)

include_directories(PUBLIC "${PROJECT_DIRECTORY}/3rd-party/spirv-cross")
add_subdirectory(${PROJECT_DIRECTORY}/3rd-party/spirv-cross EXCLUDE_FROM_ALL)

set_target_properties(spirv-cross-core PROPERTIES FOLDER "3rd-party")
set_target_properties(spirv-cross-glsl PROPERTIES FOLDER "3rd-party")
set_target_properties(spirv-cross-hlsl PROPERTIES FOLDER "3rd-party")
