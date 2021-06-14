include_directories(PUBLIC "${PROJECT_DIRECTORY}/3rd-party/hlslpp/include")
add_subdirectory(${PROJECT_DIRECTORY}/3rd-party/hlslpp)
set_target_properties(hlslpp PROPERTIES FOLDER "3rd-party")


