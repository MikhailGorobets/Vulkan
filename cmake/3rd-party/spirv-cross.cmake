add_subdirectory(${PROJECT_DIRECTORY}/3rd-party/spirv-cross EXCLUDE_FROM_ALL)

set_target_properties(spirv-cross-core    PROPERTIES FOLDER "3rd-party")
set_target_properties(spirv-cross-glsl    PROPERTIES FOLDER "3rd-party")
set_target_properties(spirv-cross-hlsl    PROPERTIES FOLDER "3rd-party")
set_target_properties(spirv-cross-reflect PROPERTIES FOLDER "3rd-party")

##target_include_directories(spirv-cross INTERFACE "${PROJECT_DIRECTORY}/3rd-party/spirv-cross")