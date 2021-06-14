set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options("/MP")
    add_definitions(-DUNICODE -D_UNICODE)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

if(WIN32)
   set(PLATFORM_WIN32 TRUE CACHE INTERNAL "Target platform: Win32")
   set(DXC_SPIRV_PATH "content/dxcompiler.dll")
   message("Target platform: Win32. SDK Version: " ${CMAKE_SYSTEM_VERSION})
endif()

if(PLATFORM_WIN32)
    set(GLOBAL_COMPILE_DEFINITIONS VK_USE_PLATFORM_WIN32_KHR=1 GLFW_EXPOSE_NATIVE_WIN32=1 NOMINMAX)
elseif(PLATFORM_LINUX)
    set(GLOBAL_COMPILE_DEFINITIONS VK_USE_PLATFORM_XCB_KHR=1 VK_USE_PLATFORM_XLIB_KHR=1)
elseif(PLATFORM_MACOS)
    set(GLOBAL_COMPILE_DEFINITIONS VK_USE_PLATFORM_MACOS_MVK=1)
elseif(PLATFORM_IOS)
    set(GLOBAL_COMPILE_DEFINITIONS VK_USE_PLATFORM_IOS_MVK=1)
elseif(PLATFORM_ANDROID)
    set(GLOBAL_COMPILE_DEFINITIONS VK_USE_PLATFORM_ANDROID_KHR=1)
else()
    message(FATAL_ERROR "Unknown platform")
endif()

add_compile_definitions(${GLOBAL_COMPILE_DEFINITIONS})