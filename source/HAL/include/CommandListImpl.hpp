#pragma once

#include <HAL/CommandList.hpp>
#include <HAL/Pipeline.hpp>
#include <vulkan/vulkan_decl.h>

namespace HAL {

    struct FrameState {
        RenderPass* pRenderPass = {};
        uint32_t    CurrentSubpass = {};
    };

    class CommandList::Internal {
    public:
        Internal(CommandAllocator const& allocator);

        auto Begin() -> void;

        auto End() -> void;

        auto BeginRenderPass(RenderPassBeginInfo const& beginInfo) -> void;

        auto EndRenderPass() -> void;
    
        auto SetComputePipeline(ComputePipeline const& pipeline, ComputeState const& state) -> void;

        auto SetGraphicsPipeline(GraphicsPipeline const& pipeline, GraphicsState const& state) -> void;
        
        auto Dispath(uint32_t x, uint32_t y, uint32_t z) -> void;

        auto GetVkCommandBuffer() const -> vk::CommandBuffer { return *m_pCommandBuffer; }

    private:
        Device*                 m_pDevice;
        vk::UniqueCommandBuffer m_pCommandBuffer;
        vk::RenderPass          m_CurrentRenderPass;
        uint32_t                m_CurrentSubpass;
    };
}