#include "../include/MemoryAllocator.hpp"


namespace HAL {
    
    MemoryAllocator::MemoryAllocator(Instance const& instance, Device const& device, AllocatorCreateInfo const& createInfo) {
        vma::DeviceMemoryCallbacks deviceMemoryCallbacks = {
            .pfnAllocate = createInfo.DeviceMemoryCallbacks.pfnAllocate,
            .pfnFree = createInfo.DeviceMemoryCallbacks.pfnFree,
            .pUserData = createInfo.DeviceMemoryCallbacks.pUserData
        };

        vma::VulkanFunctions vulkanFunctions = {
            .vkGetPhysicalDeviceProperties = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceProperties,
            .vkGetPhysicalDeviceMemoryProperties = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceMemoryProperties,
            .vkAllocateMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkAllocateMemory,
            .vkFreeMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkFreeMemory,
            .vkMapMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkMapMemory,
            .vkUnmapMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkUnmapMemory,
            .vkFlushMappedMemoryRanges = VULKAN_HPP_DEFAULT_DISPATCHER.vkFlushMappedMemoryRanges,
            .vkInvalidateMappedMemoryRanges = VULKAN_HPP_DEFAULT_DISPATCHER.vkInvalidateMappedMemoryRanges,
            .vkBindBufferMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindBufferMemory,
            .vkBindImageMemory = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindImageMemory,
            .vkGetBufferMemoryRequirements = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements,
            .vkGetImageMemoryRequirements = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements,
            .vkCreateBuffer = VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateBuffer,
            .vkDestroyBuffer = VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyBuffer,
            .vkCreateImage = VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateImage,
            .vkDestroyImage = VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyImage,
            .vkCmdCopyBuffer = VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdCopyBuffer,
            .vkGetBufferMemoryRequirements2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements2,
            .vkGetImageMemoryRequirements2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements2,
            .vkBindBufferMemory2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindBufferMemory2,
            .vkBindImageMemory2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkBindImageMemory2,
            .vkGetPhysicalDeviceMemoryProperties2KHR = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceMemoryProperties2
        };

        vma::AllocatorCreateInfo allocatorCI = {
            .flags = createInfo.Flags,
            .physicalDevice = device.GetVkPhysicalDevice(),
            .device = device.GetVkDevice(),
            .pDeviceMemoryCallbacks = &deviceMemoryCallbacks,
            .frameInUseCount = createInfo.FrameInUseCount,
            .pVulkanFunctions = &vulkanFunctions,
            .instance = instance.GetVkInstance(),
            .vulkanApiVersion = VK_API_VERSION_1_2,
        };

        m_pAllocator = vma::createAllocatorUnique(allocatorCI);
        m_FrameIndex = 0;
        m_FrameCount = createInfo.FrameInUseCount;
    }

    auto MemoryAllocator::NextFrame() -> void {
        m_FrameIndex = m_FrameIndex++ % m_FrameCount;
        m_pAllocator->setCurrentFrameIndex(m_FrameIndex);
    }
}