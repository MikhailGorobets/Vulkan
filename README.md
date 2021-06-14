<a name="cloning"></a>
# Cloning the Repository

To get the repository and all submodules, use the following command:

```
git clone --recursive https://github.com/MikhailGorobets/Vulkan.git
```

When updating existing repository, don't forget to update all submodules:

```
git pull
git submodule update --init --recursive
```

<a name="build_and_run"></a>
# Build and Run Instructions

<a name="build_and_run_win32"></a>
## Win32

Build prerequisites:

* Windows SDK 10.0.17763.0 or later
* C++ build tools

Use either CMake GUI or command line tool to generate build files. For example, to generate 
[Visual Studio 2019](https://www.visualstudio.com/vs/community) 64-bit solution and project files in *build/Win64* folder, 
navigate to the engine's root folder and run the following command:

```
cmake -S . -B ./build/Win64 -G "Visual Studio 16 2019" -A x64
```


## Supported Platforms

|  Platform                                                                                                                                         | Build status                                                                                       |
|---------------------------------------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------|
| <img src="https://user-images.githubusercontent.com/25492259/121948839-32127100-cd71-11eb-84cd-40277948cad2.png" width=22 valign="middle"> Windows | ![Build Status](https://github.com/MikhailGorobets/Vulkan/actions/workflows/windows.yml/badge.svg) |



:warning: In current implementation, full path to cmake build folder **must not contain white spaces**.

To enable Vulkan validation layers, you will need to download [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) and add environemt
variable `VK_LAYER_PATH` that contains path to the *Bin* directory in VulkanSDK installation folder.

