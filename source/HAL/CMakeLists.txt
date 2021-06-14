cmake_minimum_required (VERSION 3.19)

project(HAL CXX)

set(INCLUDE 
    include/AdapterImpl.hpp
    include/CommandAllocatorImpl.hpp
    include/CommandListImpl.hpp
    include/CommandQueueImpl.hpp
    include/ComPtr.hpp
    include/DeviceImpl.hpp
    include/FenceImpl.hpp
    include/InstanceImpl.hpp
    include/RenderPassImpl.hpp
    include/ShaderCompilerImpl.hpp
    include/SwapChainImpl.hpp
  
)

set(INTERFACE 
    interface/HAL/Adapter.hpp
    interface/HAL/Device.hpp
    interface/HAL/Instance.hpp
    interface/HAL/InternalPtr.hpp
    interface/HAL/SwapChain.hpp
    interface/HAL/ShaderCompiler.hpp
    interface/HAL/Fence.hpp
    interface/HAL/CommandQueue.hpp
    interface/HAL/CommandList.hpp
    interface/HAL/CommandAllocator.hpp
    interface/HAL/RenderPass.hpp
)


set(SOURCE
    source/AdapterImpl.cpp
    source/CommandAllocatorImpl.cpp
    source/CommandListImpl.cpp
    source/CommandQueueImpl.cpp
    source/DeviceImpl.cpp   
    source/FenceImpl.cpp    
    source/InstanceImpl.cpp
    source/RenderPassImpl.cpp
    source/ShaderCompilerImpl.cpp
    source/SwapChainImpl.cpp
)

source_group("include"   FILES ${INCLUDE})
source_group("interface" FILES ${INTERFACE})
source_group("source"    FILES ${SOURCE})

add_library(HAL ${INCLUDE} ${INTERFACE} ${SOURCE})