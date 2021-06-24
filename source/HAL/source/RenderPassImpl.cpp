#include "../include/RenderPassImpl.hpp"
#include "../include/DeviceImpl.hpp"
#include "../include/CommandListImpl.hpp"

namespace HAL {

    RenderPass::Internal::Internal(Device const& device, vk::RenderPassCreateInfo const& createInfo) {
        m_pRenderPass = device.GetVkDevice().createRenderPassUnique(createInfo);
        for (size_t index = 0; index < createInfo.attachmentCount; index++)
            m_AttachmentsFormat.push_back(createInfo.pAttachments[index].format);
    }

    auto RenderPass::Internal::GenerateFrameBufferAndCommit(vk::CommandBuffer cmdBuffer, RenderPassBeginInfo const& beginInfo) -> void {

        std::vector<vk::FramebufferAttachmentImageInfo> frameBufferAttanchments;
        std::vector<vk::ImageView>  frameBufferImageViews;
        std::vector<vk::ClearValue> frameBufferClearValues;

        frameBufferAttanchments.reserve(std::size(m_AttachmentsFormat));
        frameBufferImageViews.reserve(std::size(m_AttachmentsFormat));
        frameBufferClearValues.reserve(std::size(m_AttachmentsFormat));

        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t layerCount = 0;

        for (size_t index = 0; index < std::size(m_AttachmentsFormat); index++) {
            width = std::max(width, beginInfo.pAttachments[index].Width);
            height = std::max(height, beginInfo.pAttachments[index].Height);
            layerCount = std::max(layerCount, beginInfo.pAttachments[index].LayerCount);

            vk::FramebufferAttachmentImageInfo framebufferAttachmentImageInfo = {
                .usage = beginInfo.pAttachments[index].ImageUsage,
                .width = beginInfo.pAttachments[index].Width,
                .height = beginInfo.pAttachments[index].Height,
                .layerCount = beginInfo.pAttachments[index].LayerCount,
                .viewFormatCount = 1,
                .pViewFormats = &m_AttachmentsFormat[index]
            };
            frameBufferAttanchments.push_back(framebufferAttachmentImageInfo);
            frameBufferImageViews.push_back(beginInfo.pAttachments[index].ImageView);
            frameBufferClearValues.push_back(beginInfo.pAttachments[index].ClearValue);
        }

        FrameBufferCacheKey key = {std::move(frameBufferAttanchments), width, height, layerCount};

        vk::Framebuffer frameBuffer = {};

        if (auto iter = m_FrameBufferCache.find(key); iter == m_FrameBufferCache.end()) {
            vk::StructureChain<vk::FramebufferCreateInfo, vk::FramebufferAttachmentsCreateInfo> framebufferCI = {
                vk::FramebufferCreateInfo {
                    .flags = vk::FramebufferCreateFlagBits::eImageless,
                    .renderPass = *m_pRenderPass,
                    .attachmentCount = static_cast<uint32_t>(std::size(key.Attachments)),
                    .width = width,
                    .height = height,
                    .layers = layerCount
                },
                vk::FramebufferAttachmentsCreateInfo{
                    .attachmentImageInfoCount = static_cast<uint32_t>(std::size(key.Attachments)),
                    .pAttachmentImageInfos = std::data(key.Attachments)
                }
            };
            auto pFrameBuffer = m_pRenderPass.getOwner().createFramebufferUnique(framebufferCI.get<vk::FramebufferCreateInfo>());
            frameBuffer = pFrameBuffer.get();
            m_FrameBufferCache.emplace(key, std::move(pFrameBuffer));
        } else {
            frameBuffer = iter->second.get();
        }

        vk::StructureChain<vk::RenderPassBeginInfo, vk::RenderPassAttachmentBeginInfo> renderPassBeginInfo = {
            vk::RenderPassBeginInfo {
                .renderPass = *m_pRenderPass,
                .framebuffer = frameBuffer,
                .renderArea = vk::Rect2D {.offset = {.x = 0, .y = 0 }, .extent = {.width = width, .height = height } },
                .clearValueCount = static_cast<uint32_t>(std::size(frameBufferClearValues)),
                .pClearValues = std::data(frameBufferClearValues)
            },
            vk::RenderPassAttachmentBeginInfo {
                .attachmentCount = static_cast<uint32_t>(std::size(frameBufferImageViews)),
                .pAttachments = std::data(frameBufferImageViews)
            }
        };
        cmdBuffer.beginRenderPass(renderPassBeginInfo.get<vk::RenderPassBeginInfo>(), vk::SubpassContents::eInline);
    }
}

namespace HAL {
    RenderPass::RenderPass(Device const& pDevice, vk::RenderPassCreateInfo const& createInfo): m_pInternal(pDevice, createInfo) {}

    RenderPass::~RenderPass() = default;

    auto RenderPass::GetVkRenderPass() const -> vk::RenderPass {
        return m_pInternal->GetRenderPass();
    }
}
