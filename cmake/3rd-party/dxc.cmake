include_directories(PUBLIC "${PROJECT_DIRECTORY}/3rd-party/dxc/include")
add_subdirectory(${PROJECT_DIRECTORY}/3rd-party/dxc)
set_target_properties(dxc PROPERTIES FOLDER "3rd-party")