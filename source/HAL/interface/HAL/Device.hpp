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
        
        auto GetTransferCommandQueue() -> TransferCommandQueue const&;
                                     
        auto GetComputeCommandQueue()  -> ComputeCommandQueue const&;  
                                     
        auto GetGraphicsCommandQueue() -> GraphicsCommandQueue const&;
      
        auto WaitIdle() -> void;
         
        auto GetVkDevice() const -> vk::Device;

        auto GetVkPipelineCache() const -> vk::PipelineCache;

        auto GetVkPhysicalDevice() const -> vk::PhysicalDevice;

        auto GetVmaAllocator() const -> vma::Allocator;

    private:     
        InternalPtr<Internal, InternalSize_Device> m_pInternal;   
    };
}
 