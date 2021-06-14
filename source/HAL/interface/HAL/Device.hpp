#pragma once

#include <HAL/InternalPtr.hpp>

namespace HAL {
    struct DeviceCreateInfo {    
      
    };
    
    class Device: NonCopyable {
    public:
        class Internal;
    public:       
        Device(Instance const& instance, Adapter const& adapter, DeviceCreateInfo const& createInfo);
    
        Device(Device&&) noexcept;

        Device& operator=(Device&&) noexcept;        

        ~Device();
        
        auto GetTransferCommandQueue() -> TransferCommandQueue&;
                                     
        auto GetComputeCommandQueue()  -> ComputeCommandQueue&;  
                                     
        auto GetGraphicsCommandQueue() -> GraphicsCommandQueue&;
      
        auto WaitIdle() -> void;
         
        auto GetVkDevice() const -> vk::Device;

        auto GetVkPipelineCache() const -> vk::PipelineCache;

        auto GetVkPhysicalDevice() const -> vk::PhysicalDevice;

    private:     
        InternalPtr<Internal, InternalSize_Device> m_pInternal;   
    };
}
 