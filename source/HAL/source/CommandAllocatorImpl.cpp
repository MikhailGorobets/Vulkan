#include "../include/CommandAllocatorImpl.hpp"
#include "../include/DeviceImpl.hpp"

namespace HAL {
    CommandAllocator::Internal::Internal(Device const& device, uint32_t queueFamilyIndex) {
        m_pCommandPool = device.GetVkDevice().createCommandPoolUnique(vk::CommandPoolCreateInfo{.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer, .queueFamilyIndex = queueFamilyIndex});
        m_pDevice = const_cast<Device*>(&device);
    }

    auto CommandAllocator::Internal::GetCommandPool() const -> vk::CommandPool { return m_pCommandPool.get(); }
    
    auto CommandAllocator::Internal::GetVkDevice() const -> vk::Device { return m_pDevice->GetVkDevice(); }

    auto CommandAllocator::Internal::GetDevice() const -> Device* { return m_pDevice; }
}

namespace HAL {
    CommandAllocator::CommandAllocator(Device const& device, uint32_t queueFamily) : m_pInternal(device, queueFamily) {}

    CommandAllocator::~CommandAllocator() = default;

    auto CommandAllocator::GetVkCommandPool() const -> vk::CommandPool {
        return m_pInternal->GetCommandPool();
    }

    TransferCommandAllocator::TransferCommandAllocator(Device const& device) : CommandAllocator(device, reinterpret_cast<const Device::Internal*>(&device)->GetTransferQueueFamilyIndex()) {}

    ComputeCommandAllocator::ComputeCommandAllocator(Device const& device) : CommandAllocator(device, reinterpret_cast<const Device::Internal*>(&device)->GetComputeQueueFamilyIndex()) {}

    GraphicsCommandAllocator::GraphicsCommandAllocator(Device const& device) : CommandAllocator(device, reinterpret_cast<const Device::Internal*>(&device)->GetGraphicsQueueFamilyIndex()) {}
}