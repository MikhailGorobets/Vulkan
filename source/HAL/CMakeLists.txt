cmake_minimum_required (VERSION 3.19)

project(HAL CXX)

set(INCLUDE 
    include/AdapterImpl.hpp
    include/DeviceImpl.hpp
    include/InstanceImpl.hpp
    include/SwapChainImpl.hpp
)

set(INTERFACE 
    interface/HAL/Adapter.hpp
    interface/HAL/Device.hpp
    interface/HAL/Instance.hpp
    interface/HAL/InternalPtr.hpp
    interface/HAL/ComPtr.hpp
    interface/HAL/SwapChain.hpp
    interface/HAL/Compiler.hpp
    interface/HAL/Fence.hpp
    interface/HAL/CommandQueue.hpp
    interface/HAL/CommandList.hpp
    interface/HAL/CommandAllocator.hpp
)


set(SOURCE
    source/AdapterImpl.cpp
    SOURCE/DeviceImpl.cpp
    source/InstanceImpl.cpp
    source/SwapChainImpl.cpp
)

source_group("include"   FILES ${INCLUDE})
source_group("interface" FILES ${INTERFACE})
source_group("source"    FILES ${SOURCE})

add_library(HAL ${INCLUDE} ${INTERFACE} ${SOURCE})