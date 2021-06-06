#pragma once

#include <HAL/InternalPtr.hpp>

namespace HAL {
    struct DeviceCreateInfo {    
      
    };
    
    class Device {
    public:
        class Internal;
    public:                 
        Device(Instance const& instance, Adapter const& adapter, DeviceCreateInfo const& createInfo);

        ~Device();
        
        auto GetTransferCommandQueue() const -> const TransferCommandQueue*;
        
        auto GetComputeCommandQueue()  const -> const ComputeCommandQueue*;  

        auto GetGraphicsCommandQueue() const -> const GraphicsCommandQueue*;
      
        auto WaitIdle() -> void;
         
        auto GetVkDevice() const -> vk::Device;

        auto GetVkPipelineCache() const -> vk::PipelineCache;

        auto GetVkPhysicalDevice() const -> vk::PhysicalDevice;

    private:     
        Internal_Ptr<Internal, InternalSize_Device> m_pInternal;   
    };
}
 