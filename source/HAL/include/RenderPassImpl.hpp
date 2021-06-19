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
        struct FrameBufferCacheKey {
            std::vector<vk::FramebufferAttachmentImageInfo> FramebufferAttachment = {};
            uint32_t Width  = {};
            uint32_t Height = {};
            uint32_t Layers = {};
            
            bool operator==(const FrameBufferCacheKey& rhs) const {
                if (Width != rhs.Width || Height != rhs.Height || Layers != rhs.Layers || std::size(FramebufferAttachment) != std::size(rhs.FramebufferAttachment))
                    return false;
                
                for(size_t index = 0; index < std::size(FramebufferAttachment); index++){
                    if (FramebufferAttachment[index].usage != rhs.FramebufferAttachment[index].usage ||
                        FramebufferAttachment[index].pViewFormats[0] != rhs.FramebufferAttachment[index].pViewFormats[0] ||
                        FramebufferAttachment[index].width != rhs.FramebufferAttachment[index].width ||
                        FramebufferAttachment[index].height != rhs.FramebufferAttachment[index].height ||
                        FramebufferAttachment[index].layerCount != rhs.FramebufferAttachment[index].layerCount)
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
   
                for(size_t index = 0; index < std::size(key.FramebufferAttachment); index++) {
                    std::hash_combine(hash, static_cast<uint32_t>(key.FramebufferAttachment[index].usage));
                    std::hash_combine(hash, static_cast<uint32_t>(key.FramebufferAttachment[index].pViewFormats[0]));
                    std::hash_combine(hash, key.FramebufferAttachment[index].width);        
                    std::hash_combine(hash, key.FramebufferAttachment[index].height);
                    std::hash_combine(hash, key.FramebufferAttachment[index].layerCount);         
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
  