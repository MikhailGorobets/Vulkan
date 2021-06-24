#pragma once
#include <HAL/Device.hpp>
#include <HAL/Instance.hpp>

#include <vulkan/vulkan_decl.h>

namespace HAL {

    struct AllocatorCreateInfo {
        vma::AllocatorCreateFlags  Flags = {};
        vma::DeviceMemoryCallbacks DeviceMemoryCallbacks = {};
        uint32_t                   FrameInUseCount = {};
    };


    class MemoryAllocator {
    public:
        MemoryAllocator(Instance const& instance, Device const& device, AllocatorCreateInfo const& createInfo);

        auto NextFrame() -> void;

        auto GetVmaAllocator() const -> vma::Allocator { return *m_pAllocator; }

    private:
        vma::UniqueAllocator m_pAllocator;
        uint32_t             m_FrameIndex;
        uint32_t             m_FrameCount;
    };
}