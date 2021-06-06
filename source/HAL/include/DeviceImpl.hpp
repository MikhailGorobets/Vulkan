#pragma once

#include <HAL/Device.hpp>
#include <HAL/Instance.hpp>
#include <HAL/Adapter.hpp>
#include <HAL/CommandQueue.hpp>
#include <vulkan/vulkan_decl.h>

namespace HAL {
    class Device::Internal {
    public:
        Internal(Instance const& instance, Adapter const& adapter, DeviceCreateInfo const& createInfo);
        
        auto WaitIdle() -> void { m_pDevice->waitIdle(); }

        auto GetGraphicsQueueFamilyIndex() const -> uint32_t { return m_QueueFamilyGraphics->queueIndex; }
        
        auto GetComputeQueueFamilyIndex() const -> uint32_t { return m_QueueFamilyCompute->queueIndex; }

        auto GetTransferQueueFamilyIndex() const -> uint32_t { return m_QueueFamilyTransfer->queueIndex; }

        auto GetGraphicsCommandQueue() const -> const HAL::GraphicsCommandQueue* { return &m_QueuesGraphics[0]; }

        auto GetComputeCommandQueue()  const -> const HAL::ComputeCommandQueue* { return &m_QueuesCompute[0]; }
        
        auto GetTransferCommandQueue() const -> const HAL::TransferCommandQueue* { return &m_QueuesTransfer[0]; }  

        auto GetDevice() const -> vk::Device { return *m_pDevice; } 
    
        auto GetPhysicalDevice() const -> vk::PhysicalDevice { return m_PhysicalDevice; }

        auto GetPipelineCache() const -> vk::PipelineCache { return *m_pPipelineCache; }

    private:      
        vk::UniqueDevice        m_pDevice = {};  
        vk::PhysicalDevice      m_PhysicalDevice = {};
        vk::UniquePipelineCache m_pPipelineCache = {};
        
        std::vector<HAL::GraphicsCommandQueue> m_QueuesGraphics = {};
        std::vector<HAL::ComputeCommandQueue>  m_QueuesCompute = {};
        std::vector<HAL::TransferCommandQueue> m_QueuesTransfer = {};

        std::optional<vkx::QueueFamilyInfo> m_QueueFamilyGraphics = {}; 
        std::optional<vkx::QueueFamilyInfo> m_QueueFamilyCompute = {};  
        std::optional<vkx::QueueFamilyInfo> m_QueueFamilyTransfer = {}; 
    };
}  

