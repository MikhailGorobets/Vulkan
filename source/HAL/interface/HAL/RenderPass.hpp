#pragma once

#include <HAL/InternalPtr.hpp>
#include <vulkan/vulkan_decl.h>

namespace HAL {   
    struct RenderPassAttachmentInfo {
        vk::ImageView       ImageView = {};
        vk::ImageUsageFlags ImageUsage = {};
        vk::ClearValue      ClearValue = {};
        uint32_t            Width = {};
        uint32_t            Height = {};
        uint32_t            LayerCount = {};
    };

    class RenderPass: NonCopyable {
    public:
        class Internal;
    public:      
        RenderPass(Device const& pDevice, vk::RenderPassCreateInfo const& createInfo);
        
        RenderPass(RenderPass&&) noexcept;

        RenderPass& operator=(RenderPass&&) noexcept;

        ~RenderPass();

        auto GetVkRenderPass() const -> vk::RenderPass;

    private:      
        InternalPtr<Internal, InternalSize_RenderPass> m_pInternal;   
    };
}