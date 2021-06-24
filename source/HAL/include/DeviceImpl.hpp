#pragma once

#include <HAL/Device.hpp>
#include <HAL/Instance.hpp>
#include <HAL/Adapter.hpp>
#include <HAL/CommandQueue.hpp>

#include "MemoryAllocator.hpp"
#include "PipelineCache.hpp"

#include <vulkan/vulkan_decl.h>

namespace HAL {

    class Device::Internal {
    public:           
        Internal(Instance const& instance, Adapter const& adapter, DeviceCreateInfo const& createInfo);
        
        auto WaitIdle() const -> void { m_pDevice->waitIdle(); }

        auto GetGraphicsQueueFamilyIndex() const -> uint32_t { return m_QueueFamilyGraphics->queueIndex; }
        
        auto GetComputeQueueFamilyIndex() const -> uint32_t { return m_QueueFamilyCompute->queueIndex; }

        auto GetTransferQueueFamilyIndex() const -> uint32_t { return m_QueueFamilyTransfer->queueIndex; }

        auto GetGraphicsCommandQueue() -> HAL::GraphicsCommandQueue& { return *reinterpret_cast<HAL::GraphicsCommandQueue*>(&m_QueuesGraphics[0]); }

        auto GetComputeCommandQueue()  -> HAL::ComputeCommandQueue& { return *reinterpret_cast<HAL::ComputeCommandQueue*>(&m_QueuesCompute[0]); }
        
        auto GetTransferCommandQueue() -> HAL::TransferCommandQueue& { return *reinterpret_cast<HAL::TransferCommandQueue*>(&m_QueuesTransfer[0]); }  

        auto GetVkDevice() const -> vk::Device { return *m_pDevice; } 

        auto GetVmaAllocator() const -> vma::Allocator { return m_pAllocator->GetVmaAllocator(); }
    
        auto GetVkPhysicalDevice() const -> vk::PhysicalDevice { return m_PhysicalDevice; }

        auto GetVkPipelineCache() const -> vk::PipelineCache { return m_pPipelineCache->GetVkPipelineCache(); }

        auto GetPipelineCache() const -> PipelineCache const&;

    private:   
        vk::UniqueDevice        m_pDevice = {};  
        vk::PhysicalDevice      m_PhysicalDevice = {};

        
        std::vector<HAL::CommandQueue> m_QueuesGraphics = {};
        std::vector<HAL::CommandQueue> m_QueuesCompute = {};
        std::vector<HAL::CommandQueue> m_QueuesTransfer = {};

        std::optional<vkx::QueueFamilyInfo> m_QueueFamilyGraphics = {}; 
        std::optional<vkx::QueueFamilyInfo> m_QueueFamilyCompute = {};  
        std::optional<vkx::QueueFamilyInfo> m_QueueFamilyTransfer = {}; 

        std::unique_ptr<MemoryAllocator> m_pAllocator;
        std::unique_ptr<PipelineCache>   m_pPipelineCache;
    };
}  