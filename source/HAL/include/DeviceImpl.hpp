#pragma once

#include <HAL/Device.hpp>
#include <HAL/Adapter.hpp>
#include <vulkan/vulkan_decl.h>

namespace HAL {

    class Device::Internal {
    public:
        Internal(Instance const& pInstance, Adapter const& pAdapter, DeviceCreateInfo const& createInfo);
        
        auto Flush() -> void { m_pDevice->waitIdle(); }

        auto GetGraphicsQueueFamilyIndex() const -> uint32_t { return m_QueueFamilyGraphics->queueIndex; }
        
        auto GetComputeQueueFamilyIndex()  const -> uint32_t { return m_QueueFamilyCompute->queueIndex; }

        auto GetTransferQueueFamilyIndex() const -> uint32_t { return m_QueueFamilyTransfer->queueIndex; }

        auto GetGraphicsQueue() const -> vk::Queue { return m_QueuesGraphics[0]; }

        auto GetComputeQueue()  const -> vk::Queue { return m_QueuesCompute[0]; }
        
        auto GetTransferQueue() const -> vk::Queue { return m_QueuesTransfer[0]; }  

        auto GetDevice() const -> vk::Device { return *m_pDevice; } 
    
        auto GetPhysicalDevice() const -> vk::PhysicalDevice { return m_PhysicalDevice; }

    private:      
        vk::UniqueDevice        m_pDevice = {};  
        vk::PhysicalDevice      m_PhysicalDevice = {};
        
        std::vector<vk::Queue> m_QueuesGraphics = {};
        std::vector<vk::Queue> m_QueuesCompute = {};
        std::vector<vk::Queue> m_QueuesTransfer = {};

        std::optional<vkx::QueueFamilyInfo> m_QueueFamilyGraphics = {}; 
        std::optional<vkx::QueueFamilyInfo> m_QueueFamilyCompute = {};  
        std::optional<vkx::QueueFamilyInfo> m_QueueFamilyTransfer = {}; 
    };
}  

