#pragma once

#include <HAL/CommandList.hpp>
#include <vulkan/vulkan_decl.h>

namespace HAL {
   class CommandList::Internal {
   public:
        Internal(CommandAllocator const& allocator);
        
        auto Begin() -> void;
        
        auto End() -> void;    

        auto GetVkCommandBuffer() const -> vk::CommandBuffer { return *m_pCommandBuffer; }
    private:
        vk::UniqueCommandBuffer m_pCommandBuffer;
    };  
    
}