#pragma once

#include <HAL/CommandList.hpp>
#include <vulkan/vulkan_decl.h>

namespace HAL {
   class CommandList::Internal {
   public:
        Internal(CommandAllocator const& allocator);
        
        auto Begin() -> void;
        
        auto End() -> void;    

        auto BeginRenderPass(RenderPassBeginInfo const& beginInfo) -> void;

        auto EndRenderPass() -> void;

        auto GetVkCommandBuffer() const -> vk::CommandBuffer { return *m_pCommandBuffer; }
    private:
        vk::UniqueCommandBuffer m_pCommandBuffer;
        vk::RenderPass          m_CurrentRenderPass;
        uint32_t                m_CurrentSubpass;
    };  
    
}