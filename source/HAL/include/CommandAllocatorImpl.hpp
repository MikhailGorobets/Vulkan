#pragma once

#include <HAL/CommandAllocator.hpp>
#include <vulkan/vulkan_decl.h>

namespace HAL {

    class CommandAllocator::Internal {
    public:
        Internal(Device const& device, uint32_t queueFamilyIndex);

        auto GetCommandPool() const -> vk::CommandPool;

        auto GetVkDevice() const -> vk::Device;

        auto GetDevice() const -> Device*;

    private:      
        Device*               m_pDevice;
        vk::UniqueCommandPool m_pCommandPool = {};
    };
}