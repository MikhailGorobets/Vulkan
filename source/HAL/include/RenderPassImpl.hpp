#pragma once

#include <HAL/RenderPass.hpp>
#include <HAL/CommandList.hpp>
#include <vulkan/vulkan_decl.h>

namespace HAL {

    class RenderPass::Internal {
    public:
        Internal(Device const& device, vk::RenderPassCreateInfo const& createInfo);       
        
        auto GenerateFrameBufferAndCommit(vk::CommandBuffer cmdBuffer, RenderPassBeginInfo const& beginInfo) -> void;
          
        auto GetRenderPass() const -> vk::RenderPass { return *m_pRenderPass; }

    private:
        using FramebufferAttachments = std::vector<vk::FramebufferAttachmentImageInfo>;

        struct FrameBufferCacheKey {
            FramebufferAttachments Attachments = {};
            uint32_t Width  = {};
            uint32_t Height = {};
            uint32_t Layers = {};
            
            bool operator==(const FrameBufferCacheKey& rhs) const {
                if (Width != rhs.Width || Height != rhs.Height || Layers != rhs.Layers || std::size(Attachments) != std::size(rhs.Attachments))
                    return false;
                
                for(size_t index = 0; index < std::size(Attachments); index++){
                    if (Attachments[index].usage != rhs.Attachments[index].usage ||
                        Attachments[index].pViewFormats[0] != rhs.Attachments[index].pViewFormats[0] ||
                        Attachments[index].width != rhs.Attachments[index].width ||
                        Attachments[index].height != rhs.Attachments[index].height ||
                        Attachments[index].layerCount != rhs.Attachments[index].layerCount)
                        return false;
                }              
                return true;
            }    
        };
        
        struct FrameBufferCacheKeyHash {
            std::size_t operator()(FrameBufferCacheKey const& key) const noexcept {
                size_t hash = 0;
                std::hash_combine(hash, key.Width);
                std::hash_combine(hash, key.Height);
                std::hash_combine(hash, key.Layers);
   
                for(size_t index = 0; index < std::size(key.Attachments); index++) {
                    std::hash_combine(hash, static_cast<uint32_t>(key.Attachments[index].usage));
                    std::hash_combine(hash, static_cast<uint32_t>(key.Attachments[index].pViewFormats[0]));
                    std::hash_combine(hash, key.Attachments[index].width);
                    std::hash_combine(hash, key.Attachments[index].height);
                    std::hash_combine(hash, key.Attachments[index].layerCount);
                }
                return hash;            
            }
        };
        
        using FrameBufferCache = std::unordered_map<FrameBufferCacheKey, vk::UniqueFramebuffer, FrameBufferCacheKeyHash>;    
    private:
        FrameBufferCache          m_FrameBufferCache = {};
        vk::UniqueRenderPass      m_pRenderPass = {};
        std::vector<vk::Format>   m_AttachmentsFormat = {};
    };
}
  