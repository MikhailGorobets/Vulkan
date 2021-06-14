include_directories(PUBLIC "${PROJECT_DIRECTORY}/3rd-party/vulkan/include")
add_subdirectory(${PROJECT_DIRECTORY}/3rd-party/vulkan)
set_target_properties(vulkan PROPERTIES FOLDER "3rd-party")
