#pragma once

#define VK_NO_PROTOTYPES 
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS

#include <vulkan/vulkan.hpp>
#include <vulkan/vk_mem_alloc.hpp>

#include <fmt/core.h>
#include <fmt/format.h>

#include <optional>

namespace vkx {

    inline auto getIndexQueueFamilyGraphics(vk::PhysicalDevice device) -> std::optional<uint32_t> {    
        for(uint32_t index = 0; auto const& e: device.getQueueFamilyProperties()) {
            if (e.queueFlags & vk::QueueFlagBits::eGraphics)
                return index;
            index++;
        }
        return {};
    }

    inline auto getIndexQueueFamilyCompute(vk::PhysicalDevice device) -> std::optional<uint32_t> {
        for(uint32_t index = 0; auto const& e: device.getQueueFamilyProperties()) {
            if ((e.queueFlags & vk::QueueFlagBits::eCompute) && !(e.queueFlags & vk::QueueFlagBits::eGraphics))
                return index;
            index++;
        }
        return {};
    }

    inline auto getIndexQueueFamilyTransfer(vk::PhysicalDevice device) -> std::optional<uint32_t> {
        for(uint32_t index = 0; auto const& e: device.getQueueFamilyProperties()) {
            if ((e.queueFlags & vk::QueueFlagBits::eTransfer) && !(e.queueFlags & vk::QueueFlagBits::eCompute))
                return index;
            index++;
        }
        return {};
    }

    template<typename T>
    auto setDebugName(vk::Device device, T objectHandle, std::string const& objectName) -> void {     
    #ifdef _DEBUG
        auto objectNameCombine = fmt::format("{0}: {1}", vk::to_string(T::objectType), objectName);
        device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT{ .objectType = T::objectType, .objectHandle = reinterpret_cast<uint64_t&>(objectHandle), .pObjectName = objectNameCombine.c_str() });
    #endif
    }

}


namespace vkx {

    inline auto isInstanceLayerAvailable(std::vector<vk::LayerProperties> const& layers, const char* name) -> bool {
        for (auto const& e : layers) {
            if (std::strcmp(e.layerName, name) == 0) {
                return true;
            }
        }
        return false;
    }
    
    inline auto isInstanceExtensionAvailable(std::vector<vk::ExtensionProperties> const& extensions, const char* name) -> bool {
        for (auto const& e : extensions) {
            if (std::strcmp(e.extensionName, name) == 0) {
                return true;
            }
        }
        return false;
    }

    inline auto isDeviceExtensionAvailable(std::vector<vk::ExtensionProperties> const& extensions, const char* name) -> bool {
        for (auto const& e : extensions) {
            if (std::strcmp(e.extensionName, name) == 0) {
                return true;
            }
        }
        return false;
    }
}


namespace vkx {

    struct ImageTransition {
        vk::Image         image = {};
        vk::ImageLayout   oldLayout = {};
        vk::ImageLayout   newLayout = {};   
        vk::PipelineStageFlags srcStages = {};
        vk::PipelineStageFlags dstStages = {};
        vk::PipelineStageFlags enabledShaderStages = {};
        vk::ImageSubresourceRange subresourceRange = {};
    };

    struct BufferTransition {
        vk::Buffer      buffer = {};
        vk::AccessFlags srcAccessMask = {};
        vk::AccessFlags dstAccessMask = {};
        vk::PipelineStageFlags srcStages = {};
        vk::PipelineStageFlags dstStages = {};
        vk::PipelineStageFlags enabledShaderStages = {}; 
    };

    void imageTransition(vk::CommandBuffer cmdBuffer, ImageTransition const& translation);

    void bufferTransition(vk::CommandBuffer cmdBuffer, BufferTransition const& translation);
}


namespace vkx {
    class [[nodiscard]] DebugUtilsLabelScoped {
    public:
        DebugUtilsLabelScoped(vk::CommandBuffer& cmdBuffer, std::string const& name, vk::ArrayWrapper1D<float, 4> color): cmdBuffer(cmdBuffer) {
            cmdBuffer.beginDebugUtilsLabelEXT(vk::DebugUtilsLabelEXT{ .pLabelName = name.c_str(), .color = color });
        }
        ~DebugUtilsLabelScoped() {
            cmdBuffer.endDebugUtilsLabelEXT();
        } 
    private:
        vk::CommandBuffer& cmdBuffer;
    };
}