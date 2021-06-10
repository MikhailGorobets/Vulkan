#pragma once

#include <HAL/CommandList.hpp>
#include <vulkan/vulkan_decl.h>

namespace HAL {
   class CommandList::Internal {
   public:
        Internal(CommandAllocator const& allocator);
        
        auto Begin() const -> void;
        
        auto End() const -> void;    

        auto GetVkCommandBuffer() const -> vk::CommandBuffer;
    private:
        vk::UniqueCommandBuffer m_pCommandBuffer;
    };  
    
}