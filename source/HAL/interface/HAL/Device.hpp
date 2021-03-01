#pragma once

#include <HAL/InternalPtr.hpp>

namespace HAL {

    struct DeviceCreateInfo {    

    };
    
    class Device {
    public:
        class Internal;
    public:
        Device(Instance const& pInstance, Adapter const& pAdapter, DeviceCreateInfo const& createInfo);
        
        auto GetTransferCommandQueue() const -> TransferCommandQueue const&;
        
        auto GetComputeCommandQueue()  const -> ComputeCommandQueue const&;  

        auto GetGraphicsCommandQueue() const -> GraphicsCommandQueue const&;
      
        auto Flush() -> void;
         
        auto GetVkDevice() const -> vk::Device;

        auto GetVkPhysicalDevice() const -> vk::PhysicalDevice;

    private:     
        Internal_Ptr<Internal, InternalSize_Device> m_pInternal;   
    };
}
 