#pragma once
#include <imgui/imgui.h>  
#include <stdint.h>


namespace vk {
    class Device;
    class PhysicalDevice;
    class RenderPass;
    class CommandBuffer;
};

namespace vma {
    class Allocator;
};

IMGUI_IMPL_API void ImGui_ImplVulkan_Init(vk::Device device, vk::PhysicalDevice adapter, vma::Allocator allocator, vk::RenderPass renderPass, std::optional<uint32_t> frameIndex = std::nullopt);
IMGUI_IMPL_API void ImGui_ImplVulkan_Shutdown();
IMGUI_IMPL_API void ImGui_ImplVulkan_NewFrame(vk::CommandBuffer commandBuffer, std::optional<uint32_t> frameIndex = std::nullopt);
