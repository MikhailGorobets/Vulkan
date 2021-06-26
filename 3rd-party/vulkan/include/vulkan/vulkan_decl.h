#pragma once

#define VK_NO_PROTOTYPES 
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS

#include <vulkan/vulkan.hpp>
#include <vulkan/vk_mem_alloc.hpp>

#include <fmt/core.h>
#include <fmt/format.h>

#include <optional>


#define KILOBYTES(x) (static_cast<uint64_t>(x) << 10ULL)
#define MEGABYTES(x) (static_cast<uint64_t>(x) << 20ULL)
#define GIGABYTES(x) (static_cast<uint64_t>(x) << 30ULL)


namespace std {
    template <class T>
    inline void hash_combine(std::size_t& seed, const T& v) {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
}

namespace vkx {
    struct QueueFamilyInfo {
        uint32_t queueIndex = {};
        uint32_t queueCount = {};
    };

    inline [[nodiscard]] auto getIndexQueueFamilyGraphicsInfo(std::vector<vk::QueueFamilyProperties> const& queueFamilyProperty) -> std::optional<QueueFamilyInfo> {
        for (uint32_t index = 0; auto const& e: queueFamilyProperty) {
            if (e.queueFlags & vk::QueueFlagBits::eGraphics)
                return QueueFamilyInfo{.queueIndex = index, .queueCount = e.queueCount};
            index++;
        }
        return {};
    }

    inline [[nodiscard]] auto getIndexQueueFamilyComputeInfo(std::vector<vk::QueueFamilyProperties> const& queueFamilyProperty) -> std::optional<QueueFamilyInfo> {
        for (uint32_t index = 0; auto const& e:queueFamilyProperty) {
            if ((e.queueFlags & vk::QueueFlagBits::eCompute) && !(e.queueFlags & vk::QueueFlagBits::eGraphics))
                return QueueFamilyInfo{.queueIndex = index, .queueCount = e.queueCount};
            index++;
        }
        return {};
    }

    inline [[nodiscard]] auto getIndexQueueFamilyTransferInfo(std::vector<vk::QueueFamilyProperties> const& queueFamilyProperty) -> std::optional<QueueFamilyInfo> {
        for (uint32_t index = 0; auto const& e: queueFamilyProperty) {
            if ((e.queueFlags & vk::QueueFlagBits::eTransfer) && !(e.queueFlags & vk::QueueFlagBits::eCompute))
                return QueueFamilyInfo{.queueIndex = index, .queueCount = e.queueCount};
            index++;
        }
        return {};
    }

    inline [[nodiscard]] auto selectPhysicalDevice(std::vector<vk::PhysicalDevice> const& physicalDevices, uint32_t ID) -> std::optional<vk::PhysicalDevice> {
        auto isGraphicsAndComputeQueueSupported = [](vk::PhysicalDevice const& adapter) {
            for (const auto& queueFamilyProps : adapter.getQueueFamilyProperties()) {
                if ((queueFamilyProps.queueFlags & vk::QueueFlagBits::eGraphics) && (queueFamilyProps.queueFlags & vk::QueueFlagBits::eCompute))
                    return true;
            }
            return false;
        };

        vk::PhysicalDevice selectedPhysicalDevice = {};
        if ((ID < std::size(physicalDevices)) && isGraphicsAndComputeQueueSupported(physicalDevices[ID]))
            selectedPhysicalDevice = physicalDevices[ID];

        if (selectedPhysicalDevice == vk::PhysicalDevice{}) {
            for (auto const& adapter : physicalDevices) {
                if (isGraphicsAndComputeQueueSupported(adapter)) {
                    selectedPhysicalDevice = adapter;
                    if (adapter.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
                        break;
                }
            }
        }

        return (selectedPhysicalDevice != vk::PhysicalDevice{}) ? std::make_optional<vk::PhysicalDevice>(selectedPhysicalDevice) : std::nullopt;
    }

    inline [[nodiscard]] auto selectPresentFormat(std::vector<vk::Format> const& preferedFormats, std::vector<vk::SurfaceFormatKHR> const& supportedFormats) -> std::optional<vk::SurfaceFormatKHR> {
        for (auto const& preferedFormat : preferedFormats)
            for (auto const& supportedFormat : supportedFormats)
                if (preferedFormat == supportedFormat.format)
                    return supportedFormat;
        return {};
    }

    inline [[nodiscard]] auto selectPresentMode(std::vector<vk::PresentModeKHR> const& preferedModes, std::vector<vk::PresentModeKHR> const& supportedModes) -> std::optional<vk::PresentModeKHR> {
        for (auto const& preferedMode : preferedModes)
            for (auto const& supportedMode : supportedModes)
                if (preferedMode == supportedMode)
                    return preferedMode;
        return {};
    }

    template<typename T>
    auto setDebugName(vk::Device device, T objectHandle, std::string const& objectName) -> void {
        auto objectNameCombine = fmt::format("{0}: {1}", vk::to_string(T::objectType), objectName);
        device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT{.objectType = T::objectType, .objectHandle = reinterpret_cast<uint64_t&>(objectHandle), .pObjectName = objectNameCombine.c_str()});
    }
}


namespace vkx {

    inline [[nodiscard]] auto isInstanceLayerAvailable(std::vector<vk::LayerProperties> const& layers, std::string_view name) -> bool {
        for (auto const& e : layers)
            if (e.layerName == name)
                return true;
        return false;
    }

    inline [[nodiscard]] auto isInstanceExtensionAvailable(std::vector<vk::ExtensionProperties> const& extensions, std::string_view name) -> bool {
        for (auto const& e : extensions)
            if (e.extensionName == name)
                return true;
        return false;
    }

    inline [[nodiscard]] auto isDeviceExtensionAvailable(std::vector<vk::ExtensionProperties> const& extensions, std::string_view name) -> bool {
        for (auto const& e : extensions)
            if (e.extensionName == name)
                return true;
        return false;
    }
}


namespace vkx {

    inline bool isDepthStencilFormat(vk::Format format) {
        switch (format) {
            case vk::Format::eD16Unorm:
            case vk::Format::eD16UnormS8Uint:
            case vk::Format::eD24UnormS8Uint:
            case vk::Format::eD32Sfloat:
            case vk::Format::eD32SfloatS8Uint:
                return true;
            default:
                return false;
        }
    }

    inline bool isFormatHasStencil(vk::Format format) {
        switch (format) {
            case vk::Format::eD16UnormS8Uint:
            case vk::Format::eD24UnormS8Uint:
            case vk::Format::eD32SfloatS8Uint:
                return true;
            default:
                return false;
        }
    }

    inline vk::ImageAspectFlags getImageAspectFlags(vk::Format format) {
        switch (format) {
            case vk::Format::eD16Unorm:
            case vk::Format::eD32Sfloat:
                return vk::ImageAspectFlagBits::eDepth;
            case vk::Format::eD16UnormS8Uint:
            case vk::Format::eD24UnormS8Uint:
            case vk::Format::eD32SfloatS8Uint:
                return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
            default:
                return vk::ImageAspectFlagBits::eColor;
        }
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