#define VMA_IMPLEMENTATION
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_STATIC_VULKAN_FUNCTIONS  0

#include <vulkan/vulkan_decl.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace vkx {

    static vk::PipelineStageFlags pipelineStageFromAccessFlags(vk::AccessFlags accessFlags, vk::PipelineStageFlags enabledShaderStages) {
        vk::PipelineStageFlags stages = {};      
        while (accessFlags != vk::AccessFlagBits{}) {
            
            vk::AccessFlagBits accessFlag = vk::AccessFlagBits{ vk::AccessFlags::MaskType{ accessFlags } & (~(vk::AccessFlags::MaskType{ accessFlags } - 1)) };
            accessFlags &= ~accessFlag;
            switch (accessFlag) {
                case vk::AccessFlagBits::eIndirectCommandRead:
                    stages |= vk::PipelineStageFlagBits::eDrawIndirect;
                    break;
                case vk::AccessFlagBits::eIndexRead:
                    stages |= vk::PipelineStageFlagBits::eVertexInput;
                    break;
                case vk::AccessFlagBits::eVertexAttributeRead:
                    stages |= vk::PipelineStageFlagBits::eVertexInput;
                    break;
                case vk::AccessFlagBits::eUniformRead:
                    stages |= enabledShaderStages;
                    break;
                case vk::AccessFlagBits::eInputAttachmentRead:
                    stages |= vk::PipelineStageFlagBits::eFragmentShader;
                    break;
                case vk::AccessFlagBits::eShaderRead:
                    stages |= enabledShaderStages;
                    break;
                case vk::AccessFlagBits::eShaderWrite:
                    stages |= enabledShaderStages;
                    break;
                case vk::AccessFlagBits::eColorAttachmentRead:
                    stages |= vk::PipelineStageFlagBits::eColorAttachmentOutput;
                    break;
                case vk::AccessFlagBits::eColorAttachmentWrite:
                    stages |= vk::PipelineStageFlagBits::eColorAttachmentOutput;
                    break;
                case vk::AccessFlagBits::eDepthStencilAttachmentRead:
                    stages |= vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
                    break;
                case vk::AccessFlagBits::eDepthStencilAttachmentWrite:
                    stages |= vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
                    break;
                case vk::AccessFlagBits::eTransferRead:
                    stages |= vk::PipelineStageFlagBits::eTransfer;
                    break;
                case vk::AccessFlagBits::eTransferWrite:
                    stages |= vk::PipelineStageFlagBits::eTransfer;
                    break;
                case vk::AccessFlagBits::eHostRead:
                    stages |= vk::PipelineStageFlagBits::eHost;
                    break;
                case vk::AccessFlagBits::eHostWrite:
                    stages |= vk::PipelineStageFlagBits::eHost;
                    break;
                case vk::AccessFlagBits::eMemoryRead:
                    break;
                case vk::AccessFlagBits::eMemoryWrite:
                    break;
                case vk::AccessFlagBits::eAccelerationStructureReadKHR:
                    stages |= vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR | vk::PipelineStageFlagBits::eRayTracingShaderKHR;
                    break;
                case vk::AccessFlagBits::eAccelerationStructureWriteKHR:
                    stages |= vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR;
                    break;
                default:
                    assert(false && "Unknown memory access flag");
                    break;
            }
        }
        return stages;
    }

    static vk::AccessFlags accessMaskFromImageLayout(vk::ImageLayout layout, bool isDstMask) {
        vk::AccessFlags accessMask = {};
        switch (layout) {
            case vk::ImageLayout::eUndefined:
                if (isDstMask) assert(false && "The new layout used in a transition must not be VK_IMAGE_LAYOUT_UNDEFINED");
                break;
            case vk::ImageLayout::eGeneral:
                accessMask = vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite;
                break;
            case vk::ImageLayout::eColorAttachmentOptimal:
                accessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
                break;
            case vk::ImageLayout::eDepthStencilAttachmentOptimal:
                accessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
                break;
            case vk::ImageLayout::eDepthStencilReadOnlyOptimal:
                accessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead;
                break;
            case vk::ImageLayout::eShaderReadOnlyOptimal:
                accessMask = vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eInputAttachmentRead;
                break;
            case vk::ImageLayout::eTransferSrcOptimal:
                accessMask = vk::AccessFlagBits::eTransferRead;
                break;
            case vk::ImageLayout::eTransferDstOptimal:
                accessMask = vk::AccessFlagBits::eTransferWrite;;
                break;
            case vk::ImageLayout::ePreinitialized:
                if (!isDstMask) {
                    accessMask = vk::AccessFlagBits::eHostWrite;
                } else {
                    assert(false && "The new layout used in a transition must not be VK_IMAGE_LAYOUT_PREINITIALIZED");
                }
                break;
            case vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal:
                accessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead;
                break;
            case vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal:
                accessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead;
                break;
            case vk::ImageLayout::ePresentSrcKHR:
                accessMask = {};
                break;
            default:
                assert(false && "Unexpected image layout");
                break;
        }
        return accessMask;
    }

    void imageTransition(vk::CommandBuffer cmdBuffer, ImageTransition const& desc) {
        vk::ImageMemoryBarrier imageBarrier = {
            .srcAccessMask = accessMaskFromImageLayout(desc.oldLayout, false),
            .dstAccessMask = accessMaskFromImageLayout(desc.newLayout, true),
            .oldLayout = desc.oldLayout,
            .newLayout = desc.newLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = desc.image,
            .subresourceRange = desc.subresourceRange
        };

        vk::PipelineStageFlags srcStages = desc.srcStages;
        vk::PipelineStageFlags dstStages = desc.dstStages;

        if (srcStages == vk::PipelineStageFlags{}) {
            if (imageBarrier.oldLayout == vk::ImageLayout::ePresentSrcKHR)
                srcStages = vk::PipelineStageFlagBits::eBottomOfPipe;
            else if (imageBarrier.srcAccessMask != vk::AccessFlags{})
                srcStages = pipelineStageFromAccessFlags(imageBarrier.srcAccessMask, desc.enabledShaderStages);
            else
                srcStages = vk::PipelineStageFlagBits::eTopOfPipe;
        }

        if (dstStages == vk::PipelineStageFlags{}) {
            if (imageBarrier.newLayout == vk::ImageLayout::ePresentSrcKHR)
                dstStages = vk::PipelineStageFlagBits::eBottomOfPipe;
            else if (imageBarrier.dstAccessMask != vk::AccessFlags{})
                dstStages = pipelineStageFromAccessFlags(imageBarrier.dstAccessMask, desc.enabledShaderStages);
            else
                srcStages = vk::PipelineStageFlagBits::eTopOfPipe;
        }

        cmdBuffer.pipelineBarrier(srcStages, dstStages, {}, {}, {}, { imageBarrier });
    }

    void bufferTransition(vk::CommandBuffer cmdBuffer, BufferTransition const& desc) {
        vk::BufferMemoryBarrier bufferBarrier = {
            .srcAccessMask = desc.srcAccessMask,
            .dstAccessMask = desc.dstAccessMask,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = desc.buffer,
            .offset = 0,
            .size = VK_WHOLE_SIZE
        };
            
        vk::PipelineStageFlags srcStages = desc.srcStages;
        vk::PipelineStageFlags dstStages = desc.dstStages;
    
        if (srcStages == vk::PipelineStageFlags{}) {
            if (bufferBarrier.srcAccessMask != vk::AccessFlags{})
                srcStages = pipelineStageFromAccessFlags(bufferBarrier.srcAccessMask, desc.enabledShaderStages);
            else
                srcStages = vk::PipelineStageFlagBits::eTopOfPipe;
        }

        if (dstStages == vk::PipelineStageFlags{})
            dstStages = pipelineStageFromAccessFlags(bufferBarrier.dstAccessMask, desc.enabledShaderStages);

        cmdBuffer.pipelineBarrier(srcStages, dstStages, {}, {}, { bufferBarrier }, {});
    }
}