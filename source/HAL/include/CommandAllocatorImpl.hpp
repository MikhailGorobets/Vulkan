#pragma once

#include <vulkan/vulkan_decl.h>
#include <HAL/CommandAllocator.hpp>

namespace HAL {
    class CommandAllocator::Internal {
    public:
         Internal(Device const& device, uint32_t queueFamilyIndex);
        
         auto GetCommandPool() const -> vk::CommandPool;
    private:
        vk::UniqueCommandPool m_pCommandPool;
    };  
}