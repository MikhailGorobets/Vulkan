#pragma once

#include <HAL/Device.hpp>
#include <HAL/Instance.hpp>
#include <HAL/Adapter.hpp>
#include <HAL/CommandQueue.hpp>

#include "MemoryAllocator.hpp"
#include "PipelineCache.hpp"
#include <HAL/DescriptorTableLayout.hpp>

#include <vulkan/vulkan_decl.h>


namespace HAL {

    class DescriptorTableLayout::Internal {
    public:
        Internal(Device const& device, std::span<const PipelineResource> resources);

        Internal();

        auto GetPipelineResource(uint32_t slotID) const -> PipelineResource const&;

        auto GetVkDescriptorSetLayout() const->vk::DescriptorSetLayout;

    private:
        vk::UniqueDescriptorSetLayout                  m_pDescritptorSetLayout = {};
        std::unordered_map<uint32_t, PipelineResource> m_PipelineResources = {};
    };
}