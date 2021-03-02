#pragma once

#include <HAL/CommandQueue.hpp>
#include <vulkan/vulkan_decl.h>

namespace HAL {

    class CommandQueue::Internal {
    public:
        auto Signal(Fence const& fence, std::optional<uint64_t> value = std::nullopt) const -> void;

        auto Wait(Fence const& fence, std::optional<uint64_t> value = std::nullopt) const -> void;
        
        auto WaitIdle() const -> void;
  
        auto GetVkQueue() const -> vk::Queue;
    private:
        mutable std::mutex m_Lock;
        vk::Queue          m_Queue;   
    };
}