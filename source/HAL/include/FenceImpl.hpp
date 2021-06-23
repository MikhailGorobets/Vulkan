#pragma once

#include <HAL/Fence.hpp>
#include <HAL/Device.hpp>
#include <vulkan/vulkan_decl.h>

namespace HAL {
    class Fence::Internal {
    public:
        Internal(Device const& device, uint64_t value);
        
        auto Signal(uint64_t value) const -> void;

        auto Wait(uint64_t value) const -> void;
    
        auto GetCompletedValue() const -> uint64_t;
        
        auto GetExpectedValue() const -> uint64_t;
    
        auto Increment() -> uint64_t;

        auto IsCompleted() const -> bool;
    
        auto GetVkSemaphore() const -> vk::Semaphore;

    private:
        vk::UniqueSemaphore m_pSemaphore = {};
        uint64_t            m_ExpectedValue = 0;
    };
}